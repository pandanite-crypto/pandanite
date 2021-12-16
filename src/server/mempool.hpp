#pragma once

#include <set>
#include <map>
#include <thread>
#include <mutex>
#include <list>
#include "../core/host_manager.hpp"
#include "../core/transaction.hpp"
#include "../core/bloomfilter.hpp"
#include "blockchain.hpp"
#include "executor.hpp"
using namespace std;

class MemPool {
    public:
        MemPool(HostManager& h, BlockChain &b);
        void sync();
        ExecutionStatus addTransaction(Transaction t);
        void finishBlock(Block& block);
        bool hasTransaction(Transaction t);
        size_t size();
        std::pair<char*, size_t> getRaw();
    protected:
        list<Transaction> toSend;
        BlockChain & blockchain;
        HostManager & hosts;
        set<Transaction> transactionQueue;
        vector<std::thread> syncThread;
        std::mutex lock;
        std::mutex sendLock;
        friend void mempool_sync(MemPool& mempool);
};