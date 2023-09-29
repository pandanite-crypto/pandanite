#pragma once

#include <set>
#include <map>
#include <thread>
#include <mutex>
#include <list>
#include <map>
#include "../core/host_manager.hpp"
#include "../core/transaction.hpp"
#include "executor.hpp"
#include "../core/block.hpp"

class BlockChain;

class MemPool {
public:
    MemPool(HostManager& h, BlockChain& b);
    ~MemPool();
    void sync();
    ExecutionStatus addTransaction(Transaction t);
    void finishBlock(Block& block);
    bool hasTransaction(Transaction t);
    size_t size();
    std::pair<char*, size_t> getRaw() const;
    std::vector<Transaction> getTransactions() const;

protected:
    void mempool_sync() const;
    bool shutdown;
    std::mutex shutdownLock;
    std::map<PublicWalletAddress, TransactionAmount> mempoolOutgoing;
    BlockChain& blockchain;
    HostManager& hosts;
    std::vector<Transaction> pendingTransactions;
    std::vector<std::thread> syncThread;
    mutable std::mutex mempool_mutex;
};
