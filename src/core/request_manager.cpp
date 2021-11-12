#include "request_manager.hpp"
#include "helpers.hpp"
#include <map>
#include <iostream>
using namespace std;


RequestManager::RequestManager(string hostUrl) {
    this->blockchain = new BlockChain();
    if (hostUrl != "") this->blockchain->sync(hostUrl);
}

json RequestManager::addTransaction(json data) {
    Transaction t = Transaction(data["transaction"]);
    this->blockchain->acquire();
    ExecutionStatus status = this->blockchain->verifyTransaction(t);
    this->blockchain->release();
    json result;
    if (status != SUCCESS) {
        result["error"] = executionStatusAsString(status);
    } else {
        this->transactionQueue.push_back(t);
        result["message"] = "SUCCESS";
    }
    if(transactionQueue.size() >= MAX_TRANSACTIONS_PER_BLOCK - 1) {
        result["error"] = "Transaction queue full";
    } 
    return result;
}
json RequestManager::submitProofOfWork(json request) {
    json result;
    // build map of all public keys in transaction
    this->blockchain->acquire();
    // parse and add mining fee 
    Block newBlock(request["block"]);
    // add to the chain!
    ExecutionStatus status = this->blockchain->addBlock(newBlock);
    result["status"] = executionStatusAsString(status);
    if (status == SUCCESS) {
        this->transactionQueue.clear();
    }
    this->blockchain->release();
    return result;
}

json RequestManager::getProofOfWork() {
    json result;
    vector<Transaction> transactions;
    // send POW problem + transaction queue item
    result["transactions"] = json::array();
    for(auto p : this->transactionQueue) result["transactions"].push_back(p.toJson());
    result["lastHash"] = SHA256toString(this->blockchain->getLastHash());
    result["challengeSize"] = this->blockchain->getDifficulty();
    return result;

}
json RequestManager::getBlock(int index) {
    return this->blockchain->getBlock(index).toJson();
}
json RequestManager::getLedger() {
    json result;
    for(auto it : this->blockchain->getLedger()) {
        string hex = walletAddressToString(it.first);
        result[hex] = it.second;
    }
    return result;
}
string RequestManager::getBlockCount() {
    int count = this->blockchain->getBlockCount();
    return std::to_string(count);
}
