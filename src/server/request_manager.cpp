#include <map>
#include <iostream>
#include <future>
#include "../core/helpers.hpp"
#include "../core/logger.hpp"
#include "../core/merkle_tree.hpp"
#include "request_manager.hpp"
using namespace std;

#define NEW_BLOCK_PEER_FANOUT 8

RequestManager::RequestManager(HostManager& hosts, string ledgerPath, string blockPath, string txdbPath) : hosts(hosts) {
    this->blockchain = new BlockChain(hosts, ledgerPath, blockPath, txdbPath);
    this->mempool = new MemPool(hosts, *this->blockchain);
    this->rateLimiter = new RateLimiter(30,5); // max of 30 requests over 5 sec period 
    this->limitRequests = true;
    if (!hosts.isDisabled()) {
        this->blockchain->sync();
     
        // initialize the mempool with a random peers transactions:
        auto randomHost = hosts.sampleAllHosts(1);
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
    this->blockchain->setMemPool(this->mempool);
}

void RequestManager::enableRateLimiting(bool enabled) {
    this->limitRequests = enabled;
}

RequestManager::~RequestManager() {
    delete blockchain;
    delete mempool;
    delete rateLimiter;
}

void RequestManager::deleteDB() {
    this->blockchain->deleteDB();
}

bool RequestManager::acceptRequest(std::string& ip) {
    if (!this->limitRequests) return true;
    return this->rateLimiter->limit(ip);
}

json RequestManager::addTransaction(Transaction& t) {
    json result;
    result["status"] = executionStatusAsString(this->mempool->addTransaction(t));
    return result;
}

json RequestManager::submitProofOfWork(Block& newBlock) {
    json result;

    if (newBlock.getId() <= this->blockchain->getBlockCount()) {
        result["status"] = executionStatusAsString(INVALID_BLOCK_ID);
        return result;
    }
    // build map of all public keys in transaction
    this->blockchain->acquire();
    // add to the chain!
    ExecutionStatus status = this->blockchain->addBlock(newBlock);
    result["status"] = executionStatusAsString(status);
    this->blockchain->release();
    
    if (status == SUCCESS) {
        //pick random neighbor hosts and forward the new block to:
        set<string> neighbors = this->hosts.sampleFreshHosts(NEW_BLOCK_PEER_FANOUT);
        vector<future<void>> reqs;
        for(auto neighbor : neighbors) {
            std::thread{[neighbor, newBlock](){
                try {
                    Block a = newBlock;
                    submitBlock(neighbor, a);
                } catch(...) {
                    Logger::logStatus("Could not forward new block to " + neighbor);
                }
            }}.detach();
        }
    }
    
    return result;
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

json RequestManager::verifyTransaction(Transaction& t) {
    json response;
    Block b;
    try {
        uint32_t blockId = this->blockchain->findBlockForTransaction(t);
        b = this->blockchain->getBlock(blockId);
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

json RequestManager::getMineStatus(uint32_t blockId) {
    json result;
    Block b = this->blockchain->getBlock(blockId).toJson();
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


json RequestManager::getProofOfWork() {
    json result;
    result["lastHash"] = SHA256toString(this->blockchain->getLastHash());
    result["challengeSize"] = this->blockchain->getDifficulty();
    result["chainLength"] = this->blockchain->getBlockCount();
    result["miningFee"] = this->blockchain->getCurrentMiningFee();
    BlockHeader last = this->blockchain->getBlockHeader(this->blockchain->getBlockCount());
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

std::pair<uint8_t*, size_t> RequestManager::getRawBlockData(uint32_t blockId) {
    return this->blockchain->getRaw(blockId);
}

BlockHeader RequestManager::getBlockHeader(uint32_t blockId) {
    return this->blockchain->getBlockHeader(blockId);
}


std::pair<char*, size_t> RequestManager::getRawTransactionData() {
    return this->mempool->getRaw();
}

json RequestManager::getBlock(uint32_t blockId) {
    return this->blockchain->getBlock(blockId).toJson();
}

json RequestManager::getPeers() {
    json peers = json::array();
    for(auto h : this->hosts.getHosts()) {
        peers.push_back(h);
    }
    return peers;
}

json RequestManager::addPeer(string address, uint64_t time, string version) {
    this->hosts.addPeer(address, time, version);
    json ret;
    ret["status"] = executionStatusAsString(SUCCESS);
    return ret;
}


json RequestManager::getLedger(PublicWalletAddress w) {
    json result;
    Ledger& ledger = this->blockchain->getLedger();
    if (!ledger.hasWallet(w)) {
        result["error"] = "Wallet not found";
    } else {
        result["balance"] = ledger.getWalletValue(w);
    }
    return result;
}
string RequestManager::getBlockCount() {
    uint32_t count = this->blockchain->getBlockCount();
    return std::to_string(count);
}

string RequestManager::getTotalWork() {
    Bigint totalWork = this->blockchain->getTotalWork();
    return to_string(totalWork);
}

json RequestManager::getStats() {
    json info;
    if (this->blockchain->getBlockCount() == 1) {
        info["error"] = "Need more data";
        return info;
    }
    info["node_version"] = BUILD_VERSION;
    int coins = this->blockchain->getBlockCount()*50;
    info["node_version"] = BUILD_VERSION;
    info["num_coins"] = coins;
    info["num_wallets"] = 0;
    int blockId = this->blockchain->getBlockCount();
    info["pending_transactions"]= this->mempool->size();
    
    int idx = this->blockchain->getBlockCount();
    Block a = this->blockchain->getBlock(idx);
    Block b = this->blockchain->getBlock(idx-1);
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
