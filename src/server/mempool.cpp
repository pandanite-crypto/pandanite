#include <iostream>
#include <sstream>
#include <future>
#include <mutex>
#include <set>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include "../core/logger.hpp"
#include "../core/api.hpp"
#include "mempool.hpp"
#include "blockchain.hpp"

#define TX_BRANCH_FACTOR 10
#define MIN_FEE_TO_ENTER_MEMPOOL 1

MemPool::MemPool(HostManager& h, BlockChain& b) : hosts(h), blockchain(b) {
    shutdown = false;
}

MemPool::~MemPool() {
    shutdown = true;
}

void MemPool::mempool_sync() {
    while (!shutdown) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::vector<Transaction> txs;
        {
            std::unique_lock<std::mutex> lock(toSend_mutex);
            if (toSend.empty()) {
                continue;
            }
            txs = std::vector<Transaction>(std::make_move_iterator(toSend.begin()), std::make_move_iterator(toSend.end()));
            toSend.clear();
        }

        std::vector<Transaction> invalidTxs;
        {
            std::unique_lock<std::mutex> lock(mempool_mutex);
            for (auto it = transactionQueue.begin(); it != transactionQueue.end(); ) {
                try {
                    ExecutionStatus status = blockchain.verifyTransaction(*it);
                    if (status != SUCCESS) {
                        invalidTxs.push_back(*it);
                        it = transactionQueue.erase(it);
                    } else {
                        ++it;
                    }
                } catch (const std::exception& e) {
                    std::cout << "Caught exception: " << e.what() << '\n';
                    invalidTxs.push_back(*it);
                    it = transactionQueue.erase(it);
                }
            }
        }

        if (transactionQueue.empty()) {
            continue;
        }
        
        std::vector<std::future<bool>> reqs;
        std::set<std::string> neighbors = hosts.sampleFreshHosts(TX_BRANCH_FACTOR);
        bool all_sent = true;

        for (auto neighbor : neighbors) {
            for (const auto& tx : txs) {
                reqs.push_back(std::async(std::launch::async, [neighbor, tx]() -> bool {
                    try {
                        sendTransaction(neighbor, tx);
                        return true;
                    } catch (...) {
                        Logger::logError("MemPool::mempool_sync", "Could not send tx to " + neighbor);
                        return false;
                    }
                }));
            }
        }

        for (auto& req : reqs) {
            if (!req.get()) {
                all_sent = false;
            }
        }

        if (!all_sent) {
            std::unique_lock<std::mutex> lock(toSend_mutex);
            for (const auto& tx : txs) {
                toSend.push_back(tx);
            }
        }
    }
}

void MemPool::sync() {
    syncThread.push_back(std::thread(&MemPool::mempool_sync, this));
}

bool MemPool::hasTransaction(Transaction t) {
    std::unique_lock<std::mutex> lock(mempool_mutex);
    return transactionQueue.count(t) > 0;
}

ExecutionStatus MemPool::addTransaction(Transaction t) {
    std::unique_lock<std::mutex> lock(mempool_mutex);

    if (transactionQueue.count(t) > 0) {
        return ALREADY_IN_QUEUE;
    }

    if (t.getFee() < MIN_FEE_TO_ENTER_MEMPOOL) {
        return TRANSACTION_FEE_TOO_LOW;
    }

    ExecutionStatus status = blockchain.verifyTransaction(t);
    if (status != SUCCESS) {
        return status;
    }

    TransactionAmount outgoing = 0;
    TransactionAmount totalTxAmount = t.getAmount() + t.getFee();

    if (!t.isFee()) {
        outgoing = mempoolOutgoing[t.fromWallet()];
    }

    if (!t.isFee() && outgoing + totalTxAmount > blockchain.getWalletValue(t.fromWallet())) {
        return BALANCE_TOO_LOW;
    }

    if (transactionQueue.size() >= (MAX_TRANSACTIONS_PER_BLOCK - 1)) {
        return QUEUE_FULL;
    }

    transactionQueue.insert(t);
    mempoolOutgoing[t.fromWallet()] += totalTxAmount;
    std::unique_lock<std::mutex> toSend_lock(toSend_mutex);
    toSend.push_back(t);

    return SUCCESS;
}

size_t MemPool::size() {
    std::unique_lock<std::mutex> lock(mempool_mutex);
    return transactionQueue.size();
}

std::vector<Transaction> MemPool::getTransactions() {
    std::unique_lock<std::mutex> lock(mempool_mutex);
    std::vector<Transaction> transactions;
    for (const auto& tx : transactionQueue) {
        transactions.push_back(tx);
    }
    return transactions;
}

std::pair<char*, size_t> MemPool::getRaw() {
    std::unique_lock<std::mutex> lock(mempool_mutex);
    size_t len = transactionQueue.size() * TRANSACTIONINFO_BUFFER_SIZE;
    char* buf = (char*) malloc(len);
    int count = 0;
    
    for (const auto& tx : transactionQueue) {
        TransactionInfo t = tx.serialize();
        transactionInfoToBuffer(t, buf + count);
        count += TRANSACTIONINFO_BUFFER_SIZE;
    }

    return std::make_pair(buf, len);
}

void MemPool::finishBlock(Block& block) {
    std::unique_lock<std::mutex> lock(mempool_mutex);
    for (const auto& tx : block.getTransactions()) {
        auto it = transactionQueue.find(tx);
        if (it != transactionQueue.end()) {
            transactionQueue.erase(it);

            if (!tx.isFee()) {
                mempoolOutgoing[tx.fromWallet()] -= tx.getAmount() + tx.getFee();

                if (mempoolOutgoing[tx.fromWallet()] == 0) {
                    mempoolOutgoing.erase(tx.fromWallet());
                }
            }
        }
    }
}
