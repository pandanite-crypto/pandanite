#include <iostream>
#include <sstream>
#include <future>
#include "../core/logger.hpp"
#include "../core/api.hpp"
#include "mempool.hpp"
using namespace std;

#define TX_BRANCH_FACTOR 10

MemPool::MemPool(HostManager& h, BlockChain &b) : hosts(h), blockchain(b) {
}

void mempool_sync(MemPool& mempool) {
    while(true) {
        if (mempool.toSend.size() == 0) continue;
        Transaction t = mempool.toSend.front();
        mempool.toSend.pop_front();
        vector<future<void>> reqs;
        set<string> neighbors = mempool.hosts.sampleHosts(TX_BRANCH_FACTOR);
        for(auto neighbor : neighbors) {
            Transaction newT = t;
            reqs.push_back(std::async([neighbor, &newT](){
                try {
                    Logger::logStatus("Sending tx to " + neighbor);
                    sendTransaction(neighbor, newT);
                } catch(...) {
                    Logger::logError("MemPool::sync", "Could not send tx to " + neighbor);
                }
            }));
        }
        for(int i =0 ; i < reqs.size(); i++) {
            reqs[i].get();
        }
    }
}

void MemPool::sync() {
    this->syncThread.push_back(std::thread(mempool_sync, ref(*this)));
}

bool MemPool::hasTransaction(Transaction t) {
    bool ret = false;
    this->lock.lock();
    if (this->transactionQueue.find(t) != this->transactionQueue.end()) ret = true;
    this->lock.unlock();
    return ret;
}

ExecutionStatus MemPool::addTransaction(Transaction t) {
    ExecutionStatus status;
    if (this->hasTransaction(t)) {
        return SUCCESS;
    }
    status = this->blockchain.verifyTransaction(t);
    if (status != SUCCESS) {
        return status;
    } else {
        this->lock.lock();
        if (this->transactionQueue.size() < MAX_TRANSACTIONS_PER_BLOCK) {
            this->transactionQueue.insert(t);
            this->seenTransactions.insert(t.getNonce());
            status = SUCCESS;
        } else {
            status = QUEUE_FULL;
        }
        this->lock.unlock();
        if (status==SUCCESS) this->toSend.push_back(t);
        return status;
    }
}

size_t MemPool::size() {
    return this->transactionQueue.size();
}

std::pair<char*, size_t> MemPool::getRaw() {
    int count = 0;
    size_t len = this->transactionQueue.size() *sizeof(TransactionInfo);
    TransactionInfo* buf = (TransactionInfo*)malloc(len);
    for (auto tx : this->transactionQueue) {
        buf[count] = tx.serialize();
        count++;
    }
    return std::pair<char*, size_t>((char*)buf, len);
}

void MemPool::finishBlock(Block& block) {
    this->lock.lock();
    // erase all of this blocks included transactions from the mempool
    for(auto tx : block.getTransactions()) {
        auto it = this->transactionQueue.find(tx);
        if (it != this->transactionQueue.end()) {
            this->transactionQueue.erase(it);
        }
    }
    this->lock.unlock();
}