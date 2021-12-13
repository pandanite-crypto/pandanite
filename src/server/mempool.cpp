#include <iostream>
#include <sstream>
#include <future>
#include "../core/logger.hpp"
#include "../core/api.hpp"
#include "mempool.hpp"
using namespace std;

#define MAX_FUTURE_BLOCKS 5
#define TX_BRANCH_FACTOR 10

MemPool::MemPool(HostManager& h, BlockChain &b) : hosts(h), blockchain(b) {
}

void mempool_sync(MemPool& mempool) {
    vector<future<void>> reqs;
    for(auto t : mempool.toSend) {
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
    }
    for(int i =0 ; i < reqs.size(); i++) {
        reqs[i].get();
    }
    mempool.toSend.clear();
}


void MemPool::sync() {
    this->syncThread.push_back(std::thread(mempool_sync, ref(*this)));
}

bool MemPool::hasTransaction(Transaction t) {
    bool ret = false;
    this->lock.lock();
    int currBlockId = t.getBlockId();
    if (this->transactionQueue.find(currBlockId) != this->transactionQueue.end()) {
        if(this->transactionQueue[currBlockId].find(t) != this->transactionQueue[currBlockId].end()) {
            ret = true;
        }
    }
    this->lock.unlock();
    return ret;
}

ExecutionStatus MemPool::addTransaction(Transaction t) {
    ExecutionStatus status;
    int currBlockId = t.getBlockId();
    // If the transaction is already in our queue return immediately
    this->lock.lock();
    if (this->hasTransaction(t)) {
        return SUCCESS;
    }
    

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
                this->seenTransactions.insert(t.getNonce());
                status = SUCCESS;
            } else {
                status = QUEUE_FULL;
            }
            this->lock.unlock();
            if (status==SUCCESS) this->toSend.push_back(t);
            return status;
        }
    } else if (currBlockId > this->blockchain.getBlockCount() + 1){ // case 2: add to a future blocks queue, no verify
        if (currBlockId < this->blockchain.getBlockCount() + MAX_FUTURE_BLOCKS) {   // limit to queueing up to N blocks in future
            this->lock.lock();
            if (this->transactionQueue[currBlockId].size() < MAX_TRANSACTIONS_PER_BLOCK) {
                status = SUCCESS;
                if (t.signatureValid()) {
                    this->transactionQueue[currBlockId].insert(t);
                    this->seenTransactions.insert(t.getNonce());
                } else status = INVALID_SIGNATURE;
            } else {
                status = QUEUE_FULL;
            }
            this->lock.unlock();
            if (status==SUCCESS) this->toSend.push_back(t);
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

std::pair<char*, size_t> MemPool::getRaw(BloomFilter& seen) {
    int count = 0;
    for (auto pair : this->transactionQueue) {
        for(auto tx : pair.second) {
            if (!seen.contains(tx.getNonce())) count++;
        }
    }
    size_t len = count*sizeof(TransactionInfo);
    TransactionInfo* buf = (TransactionInfo*)malloc(len);
    count = 0;
    for (auto pair : this->transactionQueue) {
        for(auto tx : pair.second) {
            if (!seen.contains(tx.getNonce()))  {
                buf[count] = tx.serialize();
                count++;
            }        
        }
    }
    return std::pair<char*, size_t>((char*)buf, len);
}


std::pair<char*, size_t> MemPool::getRaw(int blockId) {
    if (this->transactionQueue.find(blockId) == this->transactionQueue.end()) {
        return std::pair<char*,size_t> (nullptr, 0);
    } 
    size_t len = this->transactionQueue[blockId].size()*sizeof(TransactionInfo);
    TransactionInfo* buf = (TransactionInfo*)malloc(len);
    int count = 0;
    for(auto tx : this->transactionQueue[blockId]) {
        buf[count] = tx.serialize();
        count++;
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

    // reset the bloomfilter with some probability
    if (rand()%20 == 0) {
        seenTransactions.clear();
    }
    this->lock.unlock();
}

set<Transaction>& MemPool::getTransactions(int blockId) {
    return this->transactionQueue[blockId];
}
