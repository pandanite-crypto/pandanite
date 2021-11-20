#include "mempool.hpp"
#include <iostream>
#include <sstream>
#include "logger.hpp"
#include "api.hpp"
using namespace std;

#define MAX_FUTURE_BLOCKS 5

MemPool::MemPool(HostManager& h, BlockChain &b) : hosts(h), blockchain(b) {
}

void mempool_sync(MemPool& mempool) {
    while (true) {
        Logger::logStatus("Syncing MemPool");
        try {
            // get rid of any un-needed blocks:
            mempool.lock.lock();
            int blockId = mempool.blockchain.getBlockCount();
            vector<int> toDelete;
            for (auto pair : mempool.transactionQueue) {
                if (pair.first <= blockId) {
                    toDelete.push_back(pair.first);
                }
            }
            for (auto blockId : toDelete) {
                mempool.transactionQueue.erase(blockId);
            }
            mempool.lock.unlock();
            // fetch mempool state from other hots
            for (auto host : mempool.hosts.getHosts()) {
                mempool.lock.lock();
                int count = 0;
                readRawTransactions(host, [&mempool, &count](Transaction t) {
                    mempool.addTransaction(t);
                    count++;
                });
                mempool.lock.unlock();
                stringstream s;
                s<<"Read "<<count<<" transactions from "<<host;
                Logger::logStatus(s.str());
            }
        } catch (const std::exception &e) {
            Logger::logError("MemPool::sync", "Failed to load tx" + string(e.what()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}


void MemPool::sync() {
    this->syncThread.push_back(std::thread(mempool_sync, ref(*this)));
}

ExecutionStatus MemPool::addTransaction(Transaction t) {
    ExecutionStatus status;
    int currBlockId = t.getBlockId();
    // If the transaction is already in our queue return immediately
    this->lock.lock();
    if (this->transactionQueue.find(currBlockId) != this->transactionQueue.end()) {
        if(this->transactionQueue[currBlockId].find(t) != this->transactionQueue[currBlockId].end()) {
            this->lock.unlock();
            return SUCCESS;
        }
    }
    this->lock.unlock();

    if (currBlockId <= this->blockchain.getBlockCount()) {
        return EXPIRED_TRANSACTION;
    } else if (currBlockId == this->blockchain.getBlockCount() + 1) { // case 1: add to latest block queue + verify
        status = this->blockchain.verifyTransaction(t);
        if (status != SUCCESS) {
            return status;
        } else {
            this->lock.lock();
            if (this->transactionQueue[currBlockId].size() < MAX_TRANSACTIONS_PER_BLOCK) {
                this->transactionQueue[currBlockId].insert(t);
                status = SUCCESS;
            } else {
                status = QUEUE_FULL;
            }
            this->lock.unlock();
            return status;
        }
    } else if (currBlockId > this->blockchain.getBlockCount() + 1){ // case 2: add to a future blocks queue, no verify
        if (currBlockId < this->blockchain.getBlockCount() + MAX_FUTURE_BLOCKS) {   // limit to queueing up to N blocks in future
            this->lock.lock();
            if (this->transactionQueue[currBlockId].size() < MAX_TRANSACTIONS_PER_BLOCK) {
                status = SUCCESS;
                this->transactionQueue[currBlockId].insert(t);
            } else {
                status = QUEUE_FULL;
            }
            this->lock.unlock();
            return status;
        } else {
            return BLOCK_ID_TOO_LARGE;
        }
    }
    return UNKNOWN_ERROR;
}

std::pair<char*, size_t> MemPool::getRaw() {
    int count = 0;
    for (auto pair : this->transactionQueue) {
        for(auto tx : pair.second) {
            count++;
        }
    }
    size_t len = count*sizeof(TransactionInfo);
    TransactionInfo* buf = (TransactionInfo*)malloc(len);
    count = 0;
    for (auto pair : this->transactionQueue) {
        for(auto tx : pair.second) {
            buf[count] = tx.serialize();
            count++;
        }
    }
    return std::pair<char*, size_t>((char*)buf, len);
}

void MemPool::finishBlock(int blockId) {
    this->lock.lock();
    // erase all transactions prior to this block
    vector<int> toDelete;
    for (auto pair : this->transactionQueue) {
        if (pair.first <= blockId) {
            toDelete.push_back(pair.first);
        }
    }
    for (auto blockId : toDelete) {
        this->transactionQueue.erase(blockId);
    }
    // validate transactions scheduled for the next block if needed:
    if (this->transactionQueue.find(blockId+1) != this->transactionQueue.end()) {
        set<Transaction>& currTransactions =  this->transactionQueue[blockId+1];
        for (auto it = currTransactions.begin(); it != currTransactions.end(); ) {
            ExecutionStatus status = this->blockchain.verifyTransaction(*it);
            if (status != SUCCESS) {
                currTransactions.erase(it++);
            } else {
                ++it;
            }
        }
    }
    this->lock.unlock();
}

set<Transaction>& MemPool::getTransactions(int blockId) {
    return this->transactionQueue[blockId];
}
