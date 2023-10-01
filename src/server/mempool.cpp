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

MemPool::MemPool(HostManager &h, BlockChain &b) : 
    shutdown(false),
    hosts(h), blockchain(b)
{
}

MemPool::~MemPool()
{
    shutdown = true;
}

void MemPool::mempool_sync() const {

    const int MAX_RETRIES = 3;

    std::map<std::string,std::future<bool>> pendingPushes; // maps from node to future

    while (!shutdown)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        auto peers {hosts.sampleFreshHosts(TX_BRANCH_FACTOR)};
        const auto N{pendingTransactions.size()};
        const auto batchSize{std::min(10ul,N)};



        // clean up completed pushes
        for (auto it = pendingPushes.begin(); it!= pendingPushes.end();){
            auto& future{it->second};
            std::future_status status{future.wait_for(std::chrono::seconds(0))};

            // continue if there is a pending send to that node
            if (status != std::future_status::ready) {
                ++it;
                continue;
            }
            future.get(); // should return immediately because status == ready
            pendingPushes.erase(it++);
        }


        std::unique_lock<std::mutex> lock(mempool_mutex);

        // send each peer a different random set of mempool
        for (auto& peer: peers) {
            auto randomOffset=rand() % (N+1);
            std::vector<Transaction> sampledTransactions;
	    for (size_t i = 0; i < batchSize; ++i) {
    		auto index = (randomOffset + i) % N;
    		sampledTransactions.push_back(pendingTransactions[index]);
	    }

            // don't send again while still pending send
            if (pendingPushes.count(peer)>0)
                continue;
            pendingPushes[peer] = std::async(std::launch::async, [peer,sampledTransactions = std::move(sampledTransactions)]() -> bool {
                        try {
                            for (auto &tx : sampledTransactions) {
                                sendTransaction(peer, tx);
                            }
                            return true;
                        } catch (...) {
                            Logger::logError("Failed to send tx to ", peer);
                            return false;
                        }
            });
        };
    }

    // wait until all child push threads terminated
    // to avoid bugs due to captured variables by reference
    for (auto &push : pendingPushes) {
        push.second.get();
    }
}

void MemPool::sync()
{
    syncThread.push_back(std::thread(&MemPool::mempool_sync, this));
}

bool MemPool::hasTransaction(Transaction t)
{
    std::unique_lock<std::mutex> lock(mempool_mutex);
    auto it{std::find(pendingTransactions.begin(),pendingTransactions.end(), t)};
    return (it!=pendingTransactions.end());
}

ExecutionStatus MemPool::addTransaction(Transaction t)
{
    std::unique_lock<std::mutex> lock(mempool_mutex);

    auto it{std::find(pendingTransactions.begin(),pendingTransactions.end(), t)};
    if (it!=pendingTransactions.end()){
        return ALREADY_IN_QUEUE;
    }

    if (t.getFee() < MIN_FEE_TO_ENTER_MEMPOOL)
    {
        return TRANSACTION_FEE_TOO_LOW;
    }

    ExecutionStatus status = blockchain.verifyTransaction(t);
    if (status != SUCCESS)
    {
        return status;
    }

    TransactionAmount outgoing = 0;
    TransactionAmount totalTxAmount = t.getAmount() + t.getFee();

    if (!t.isFee())
    {
        outgoing = mempoolOutgoing[t.fromWallet()];
    }

    if (!t.isFee() && outgoing + totalTxAmount > blockchain.getWalletValue(t.fromWallet()))
    {
        return BALANCE_TOO_LOW;
    }

    if (pendingTransactions.size() >= (MAX_TRANSACTIONS_PER_BLOCK - 1))
    {
        return QUEUE_FULL;
    }

    pendingTransactions.push_back(t);
    mempoolOutgoing[t.fromWallet()] += totalTxAmount;

    return SUCCESS;
}

size_t MemPool::size()
{
    std::unique_lock<std::mutex> lock(mempool_mutex);
    return pendingTransactions.size();
}

std::vector<Transaction> MemPool::getTransactions() const
{
    std::unique_lock<std::mutex> lock(mempool_mutex);
    std::vector<Transaction> transactions;
    for (const auto &tx : pendingTransactions)
    {
        transactions.push_back(tx);
    }
    return transactions;
}

std::pair<char *, size_t> MemPool::getRaw() const
{
    std::unique_lock<std::mutex> lock(mempool_mutex);
    size_t len = pendingTransactions.size() * TRANSACTIONINFO_BUFFER_SIZE;
    char *buf = (char *)malloc(len);
    int count = 0;

    for (const auto &tx : pendingTransactions)
    {
        TransactionInfo t = tx.serialize();
        transactionInfoToBuffer(t, buf + count);
        count += TRANSACTIONINFO_BUFFER_SIZE;
    }

    return std::make_pair(buf, len);
}

void MemPool::finishBlock(Block &block)
{
    std::unique_lock<std::mutex> lock(mempool_mutex);
    for (const auto &tx : block.getTransactions())
    {
        auto it{std::find(pendingTransactions.begin(),pendingTransactions.end(), tx)};
        if (it != pendingTransactions.end())
        {
            pendingTransactions.erase(it);

            if (!tx.isFee())
            {
                mempoolOutgoing[tx.fromWallet()] -= tx.getAmount() + tx.getFee();

                if (mempoolOutgoing[tx.fromWallet()] == 0)
                {
                    mempoolOutgoing.erase(tx.fromWallet());
                }
            }
        }
    }
}
