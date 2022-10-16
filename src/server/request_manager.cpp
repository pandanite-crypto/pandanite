#include <map>
#include <iostream>
#include <future>
#include <math.h>
#include "../core/helpers.hpp"
#include "../core/logger.hpp"
#include "../shared/api.hpp"
#include "../core/merkle_tree.hpp"
#include "request_manager.hpp"

using namespace std;

#define NEW_BLOCK_PEER_FANOUT 8

RequestManager::RequestManager(json config) {
    this->config = config;
    if (config["mockHosts"]) {
        this->hosts = std::make_shared<HostManager>();
    } else {
        this->hosts = std::make_shared<HostManager>(config);
    }

    this->defaultProgram = std::make_shared<Program>(config);

    this->hosts->setBlockstore(this->defaultProgram->getBlockstore());
    
    // start downloading headers from peers
    this->hosts->syncHeadersWithPeers();
    // start pinging other peers about ourselves
    this->hosts->startPingingPeers();

    auto blockchain = std::make_shared<BlockChain>(this->defaultProgram, *this->hosts);
    this->subchains[NULL_SHA256_HASH] = blockchain;
    this->mempool = std::make_shared<MemPool>(*this->hosts, *blockchain);
    this->rateLimiter = std::make_shared<RateLimiter>(30,5); // max of 30 requests over 5 sec period 
    this->programs = std::make_shared<ProgramStore>(config);
    string path = config["storagePath"];
    this->programs->init(path);
    this->limitRequests = true;
    


    if (!this->hosts->isDisabled()) {
        blockchain->sync();
     
        // initialize the mempool with a random peers transactions:
        auto randomHost = hosts->sampleFreshHosts(1);
        if (randomHost.size() > 0) {
            try {
                string host = *randomHost.begin();
                vector<Transaction> transactions;
                readRawTransactions(host, transactions);
                for(auto& t : transactions) {
                    mempool->addTransaction(t);
                }
            } catch(...) {}
        }
    }
    this->mempool->sync();
    blockchain->setMemPool(this->mempool);

    // add all the program as virtual chains
    vector<ProgramID> programs = this->programs->getProgramIds();
    for(auto programId : programs) {
        std::shared_ptr<Program> p = this->programs->getProgram(programId);
        this->setProgram(p);
    }
}

json RequestManager::getConfig() {
    return this->config;
}

void RequestManager::enableRateLimiting(bool enabled) {
    this->limitRequests = enabled;
}

string RequestManager::getHostAddress() {
    return this->hosts->getAddress();
}

std::shared_ptr<VirtualChain> RequestManager::getChain(ProgramID program) {
    return this->subchains[program];
}

json RequestManager::getPeerStats() {
    json ret;
    std::shared_ptr<BlockChain> blockchain = this->getMainChain();
    for(auto elem : blockchain->getHeaderChainStats()) {
        ret[elem.first] = elem.second;
    }
    return ret;
}

void RequestManager::exit() {
    this->defaultProgram->closeDB();
}

RequestManager::~RequestManager() {
}

void RequestManager::deleteDB() {
    this->defaultProgram->deleteData();
}

std::shared_ptr<BlockChain> RequestManager::getMainChain() {
    std::shared_ptr<BlockChain> blockchain = std::dynamic_pointer_cast<BlockChain>(this->getChain(NULL_SHA256_HASH));
    return blockchain;
}

json RequestManager::getTransactionsForWallet(PublicWalletAddress addr, ProgramID program) {
    if (program != NULL_SHA256_HASH) throw std::runtime_error("Cannot get balance for non default program");
    json ret = json::array();
    vector<Transaction> txs = this->getMainChain()->getTransactionsForWallet(addr);
    for(auto tx : txs) {
        ret.push_back(tx.toJson());
    }
    return ret;
}

bool RequestManager::acceptRequest(std::string& ip) {
    if (!this->limitRequests) return true;
    return this->rateLimiter->limit(ip);
}

json RequestManager::addTransaction(Transaction& t, ProgramID program) {
    json result;
    result["status"] = executionStatusAsString(this->mempool->addTransaction(t));
    return result;
}

json RequestManager::submitProofOfWork(Block& newBlock, ProgramID program) {
    json result;
    ExecutionStatus status;
    std::shared_ptr<VirtualChain> chain = this->getChain(program);

    if (newBlock.getId() <= chain->getBlockCount()) {
        result["status"] = executionStatusAsString(INVALID_BLOCK_ID);
        return result;
    }
    // build map of all public keys in transaction
    // add to the chain!
    status = chain->addBlockSync(newBlock);
    result["status"] = executionStatusAsString(status);

    if (status == SUCCESS) {
        //pick random neighbor hosts and forward the new block to:
        set<string> neighbors = this->hosts->sampleFreshHosts(NEW_BLOCK_PEER_FANOUT);
        vector<future<void>> reqs;
        for(auto neighbor : neighbors) {
            std::thread{[neighbor, newBlock, program](){
                try {
                    Block a = newBlock;
                    submitBlock(neighbor, a, program);
                } catch(...) {
                    Logger::logStatus("Could not forward new block to " + neighbor);
                }
            }}.detach();
        }
    }
    
    return result;
}

json RequestManager::getInfo(json args, ProgramID program) {
    auto chain = this->getChain(program);
    return chain->getInfo(args);
}

json hashTreeToJson(shared_ptr<HashTree> root) {
    json ret;
    ret["hash"] = SHA256toString(root->hash);
    if (root->left) {
        ret["left"] = hashTreeToJson(root->left);
    }
    if (root->right) {
        ret["right"] = hashTreeToJson(root->right);
    }
    return ret;
}

json RequestManager::getTransactionStatus(SHA256Hash txid, ProgramID program) {
    json response;
    Block b;
    try {
        auto chain = this->getChain(program);
        uint32_t blockId = chain->findBlockForTransactionId(txid);
        b = chain->getBlock(blockId);
        response["status"] = "IN_CHAIN";
        response["blockId"] = b.getId();
    } catch(...) {
        response["status"] = "NOT_IN_CHAIN";
        response["blockId"] = -1;
    }
    return response;  
}

json RequestManager::verifyTransaction(Transaction& t, ProgramID program) {
    json response;
    Block b;
    try {
        auto chain = this->getChain(program);
        uint32_t blockId = chain->findBlockForTransaction(t);
        b = chain->getBlock(blockId);
        MerkleTree m;
        m.setItems(b.getTransactions());
        shared_ptr<HashTree> root = m.getMerkleProof(t);
        if (root == NULL) {
            response["error"] = "Could not find transaction in block";
        } else {
            response["status"] = "SUCCESS";
            response["proof"] = hashTreeToJson(root);
        }
    } catch(...) {
        response["error"] = "Could not find block";
    }
    return response;
}

json RequestManager::getMineStatus(uint32_t blockId, ProgramID program) {
    json result;
    auto chain = this->getChain(program);
    Block b = chain->getBlock(blockId).toJson();
    PublicWalletAddress minerAddress;
    TransactionAmount txFees = 0;
    TransactionAmount mintFee = 0;
    for(auto & t : b.getTransactions()) {
        if (t.isFee()) {
            minerAddress = t.toWallet();
            mintFee = t.getAmount();
        } else {
            txFees += t.getTransactionFee();
        }
    }
    result["minerWallet"] = walletAddressToString(minerAddress);
    result["mintFee"] = mintFee;
    result["txFees"] = txFees;
    result["timestamp"] = uint64ToString(b.getTimestamp());
    return result;
}

json RequestManager::getProgram(ProgramID program) {
    if (!this->programs->hasProgram(program)) {
        json result;
        result["error"] = "Program not loaded in node";
        return result;
    }
    return this->programs->getProgram(program)->toJson();
}

json RequestManager::setProgram(std::shared_ptr<Program> program) {
    json result;
    this->programs->insertProgram(*program);
    this->subchains[program->getId()] = std::make_shared<VirtualChain>(program, *hosts);
    this->subchains[program->getId()]->sync();
    result["status"] = executionStatusAsString(SUCCESS);
    return result;
}

json RequestManager::getProofOfWork(ProgramID program) {
    json result;
    auto chain = this->getChain(program);
    result["lastHash"] = SHA256toString(chain->getLastHash());
    result["challengeSize"] = chain->getDifficulty();
    result["chainLength"] = chain->getBlockCount();
    result["miningFee"] = chain->getCurrentMiningFee();
    BlockHeader last = chain->getBlockHeader(chain->getBlockCount());
    result["lastTimestamp"] = uint64ToString(last.timestamp);
    return result;
}

json RequestManager::getTransactionQueue() {
    json ret = json::array();
    for(auto & tx : this->mempool->getTransactions()) {
        ret.push_back(tx.toJson());
    }
    return ret;
}

std::pair<uint8_t*, size_t> RequestManager::getRawBlockData(uint32_t blockId, ProgramID program) {
    auto chain = this->getChain(program);
    return chain->getRaw(blockId);
}

BlockHeader RequestManager::getBlockHeader(uint32_t blockId, ProgramID program) {
    auto chain = this->getChain(program);
    return chain->getBlockHeader(blockId);
}

std::pair<char*, size_t> RequestManager::getRawTransactionData() {
    return this->mempool->getRaw();
}

json RequestManager::getBlock(uint32_t blockId, ProgramID program) {
    auto chain = this->getChain(program);
    return chain->getBlock(blockId).toJson();
}

json RequestManager::getPeers(ProgramID program) {
    json peers = json::array();
    for(auto h : this->hosts->getHosts()) {
        peers.push_back(h);
    }
    return peers;
}

json RequestManager::addPeer(string address, uint64_t time, string version, string network) {
    this->hosts->addPeer(address, time, version, network);
    json ret;
    ret["status"] = executionStatusAsString(SUCCESS);
    return ret;
}


json RequestManager::getLedger(PublicWalletAddress w, ProgramID program) {
    json result;
    try {
        auto chain = this->getChain(program);
        result["balance"] = chain->getWalletValue(w);
    } catch(...) {
        result["error"] = "Wallet not found";
    }
    return result;
}
string RequestManager::getBlockCount(ProgramID program) {
    auto chain = this->getChain(program);
    uint32_t count = chain->getBlockCount();
    return std::to_string(count);
}

string RequestManager::getTotalWork(ProgramID program) {
    auto chain = this->getChain(program);
    Bigint totalWork = chain->getTotalWork();
    return to_string(totalWork);
}

uint64_t RequestManager::getNetworkHashrate() {
    auto blockCount = this->getMainChain()->getBlockCount();

    uint64_t totalWork = 0;

    int blockStart = blockCount < 52 ? 2 : blockCount - 50;
    int blockEnd = blockCount;

    int start = 0;
    int end = 0;

    for (int blockId = blockStart; blockId <= blockEnd; blockId++) {
        auto header = this->getMainChain()->getBlockHeader(blockId);

        if (blockId == blockStart) {
            start = header.timestamp;
        }

        if (blockId == blockEnd) {
            end = header.timestamp;
        }

        totalWork += pow(2, header.difficulty);
    }

    return totalWork / (end - start);
}

json RequestManager::getStats() {
    json info;
    if (this->getMainChain()->getBlockCount() == 1) {
        info["error"] = "Need more data";
        return info;
    }
    info["node_version"] = BUILD_VERSION;
    int coins = this->getMainChain()->getBlockCount()*50;
    info["node_version"] = BUILD_VERSION;
    info["num_coins"] = coins;
    info["num_wallets"] = 0;
    int blockId = this->getMainChain()->getBlockCount();
    info["pending_transactions"]= this->mempool->size();
    
    int idx = this->getMainChain()->getBlockCount();
    Block a = this->getMainChain()->getBlock(idx);
    Block b = this->getMainChain()->getBlock(idx-1);
    int timeDelta = a.getTimestamp() - b.getTimestamp();
    int totalSent = 0;
    int fees = 0;
    info["transactions"] = json::array();
    for(auto t : a.getTransactions()) {
        totalSent += t.getAmount();
        fees += t.getTransactionFee();
        info["transactions"].push_back(t.toJson());
    }
    int count = a.getTransactions().size();
    info["transactions_per_second"]= a.getTransactions().size()/(double)timeDelta;
    info["transaction_volume"]= totalSent;
    info["avg_transaction_size"]= totalSent/count;
    info["avg_transaction_fee"]= fees/count;
    info["difficulty"]= a.getDifficulty();
    info["current_block"]= a.getId();
    info["last_block_time"]= timeDelta;
    return info;
}
