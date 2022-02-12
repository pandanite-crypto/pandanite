#include <iostream>
#include <sstream>
#include <future>
#include "../core/logger.hpp"
#include "../core/api.hpp"
#include "mempool.hpp"
#include "blockchain.hpp"
using namespace std;

#define TX_BRANCH_FACTOR 10

MemPool::MemPool(HostManager& h, BlockChain &b) : hosts(h), blockchain(b) {
}

void mempool_sync(MemPool& mempool) {
    while(true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        Transaction t;
        mempool.sendLock.lock();
        if (mempool.toSend.size() == 0) {
            mempool.sendLock.unlock();
            continue;
        } else {
            t = mempool.toSend.front();
            mempool.toSend.pop_front();
        }
        mempool.sendLock.unlock();
        vector<future<void>> reqs;
        set<string> neighbors = mempool.hosts.sampleFreshHosts(TX_BRANCH_FACTOR);
        for(auto neighbor : neighbors) {
            Transaction newT = t;
            reqs.push_back(std::async([neighbor, &newT](){
                try {
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
    this->lock.lock();
    if (this->transactionQueue.find(t) != this->transactionQueue.end()) {
        status = ALREADY_IN_QUEUE;
        this->lock.unlock();
        return status;
    }
    
    status = this->blockchain.verifyTransaction(t);
    if (status != SUCCESS) {
        this->lock.unlock();
        return status;
    } else {
        // check how much of the from wallets balance is outstanding in mempool
        TransactionAmount outgoing = 0;
        if (!t.isFee()) outgoing = this->mempoolOutgoing[t.fromWallet()];

        if (!t.isFee() && outgoing + t.getAmount() > this->blockchain.getWalletValue(t.fromWallet())) {
            status = BALANCE_TOO_LOW;
        } else  if (this->transactionQueue.size() < MAX_TRANSACTIONS_PER_BLOCK) {
            this->transactionQueue.insert(t);
            this->mempoolOutgoing[t.fromWallet()] += t.getAmount();
            status = SUCCESS;
        } else {
            status = QUEUE_FULL;
        }
        if (status==SUCCESS) {
            this->sendLock.lock();
            this->toSend.push_back(t);
            this->sendLock.unlock();
        }
        this->lock.unlock();
        return status;
    }
}

size_t MemPool::size() {
    return this->transactionQueue.size();
}

vector<Transaction> MemPool::getTransactions() {
    vector<Transaction> transactions;
    for(auto tx : this->transactionQueue) {
        transactions.push_back(tx);
    }
    return std::move(transactions);
}

std::pair<char*, size_t> MemPool::getRaw() {
    int count = 0;
    this->lock.lock();
    size_t len = this->transactionQueue.size() * TRANSACTIONINFO_BUFFER_SIZE;
    char* buf = (char*) malloc(len);
    for (auto tx : this->transactionQueue) {
        TransactionInfo t = tx.serialize();
        transactionInfoToBuffer(t, buf + count);
        count+=TRANSACTIONINFO_BUFFER_SIZE;
    }
    this->lock.unlock();
    return std::pair<char*, size_t>((char*)buf, len);
}

void MemPool::finishBlock(Block& block) {
    this->lock.lock();
    // erase all of this blocks included transactions from the mempool
    for(auto tx : block.getTransactions()) {
        auto it = this->transactionQueue.find(tx);
        if (it != this->transactionQueue.end()) {
            this->transactionQueue.erase(it);
            if (!tx.isFee()) {
                this->mempoolOutgoing[tx.fromWallet()] -= tx.getAmount();
                if (this->mempoolOutgoing[tx.fromWallet()] == 0) {
                    // remove the key if there is no longer an outgoing amount
                    auto it = this->mempoolOutgoing.find(tx.fromWallet());
                    this->mempoolOutgoing.erase(it);
                }
            }
        }
    }
    this->lock.unlock();
}