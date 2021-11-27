#include "request_manager.hpp"
#include "helpers.hpp"
#include "user.hpp"
#include "logger.hpp"
#include "merkle_tree.hpp"

#include <map>
#include <iostream>
using namespace std;


RequestManager::RequestManager(HostManager& hosts) : hosts(hosts) {
    this->blockchain = new BlockChain(hosts);
    this->mempool = new MemPool(hosts, *this->blockchain);
    if (hosts.size()>0) {
        this->blockchain->sync();
        this->mempool->sync();
    }
}

void RequestManager::deleteDB() {
    this->blockchain->deleteDB();
}

json RequestManager::addTransaction(Transaction& t) {
    json result;
    result["status"] = executionStatusAsString(this->mempool->addTransaction(t));
    return result;
}

json RequestManager::submitProofOfWork(Block& newBlock) {
    json result;
    // build map of all public keys in transaction
    this->blockchain->acquire();
    // add to the chain!
    ExecutionStatus status = this->blockchain->addBlock(newBlock);
    result["status"] = executionStatusAsString(status);
    if (status == SUCCESS) {
        this->mempool->finishBlock(newBlock.getId());
    }
    this->blockchain->release();
    return result;
}

json hashTreeToJson(HashTree* root) {
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
        b = this->blockchain->getBlock(t.getBlockId());
        MerkleTree m;
        m.setItems(b.getTransactions());
        HashTree* root = m.getMerkleProof(t);
        if (root == NULL) {
            response["error"] = "Could not find transaction in block";
        } else {
            // convert to a JSON tree of hashes
            response["status"] = "SUCCESS";
            response["proof"] = hashTreeToJson(root);
            delete root;
        }
    } catch(...) {
        response["error"] = "Could not find block";
    }
    return response;
}

json RequestManager::getProofOfWork() {
    json result;
    vector<Transaction> transactions;
    result["lastHash"] = SHA256toString(this->blockchain->getLastHash());
    result["challengeSize"] = this->blockchain->getDifficulty();
    return result;

}

std::pair<char*, size_t> RequestManager::getRawBlockData(int index) {
    return this->blockchain->getRaw(index);
}

std::pair<char*, size_t> RequestManager::getRawTransactionData(BloomFilter & seen) {
    return this->mempool->getRaw(seen);
}

std::pair<char*, size_t> RequestManager::getRawTransactionDataForBlock(int blockId) {
    return this->mempool->getRawForBlock(blockId);
}

json RequestManager::getBlock(int index) {
    return this->blockchain->getBlock(index).toJson();
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
    int count = this->blockchain->getBlockCount();
    return std::to_string(count);
}

json RequestManager::getStats() {
    json info;
    if (this->blockchain->getBlockCount() == 1) {
        info["error"] = "Need more data";
        return info;
    }
    int coins = this->blockchain->getBlockCount()*50;
    int wallets = this->blockchain->getLedger().size();
    info["num_coins"] = coins;
    info["num_wallets"] = wallets;
    int blockId = this->blockchain->getBlockCount();
    info["pending_transactions"]= this->mempool->getTransactions(blockId + 1).size();
    
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
