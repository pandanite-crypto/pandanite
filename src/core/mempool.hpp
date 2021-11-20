#pragma once
#include "host_manager.hpp"
#include "transaction.hpp"
#include "blockchain.hpp"
#include "executor.hpp"
#include <set>
#include <map>
#include <thread>
#include <mutex>
using namespace std;

class MemPool {
    public:
        MemPool(HostManager& h, BlockChain &b);
        void sync();
        ExecutionStatus addTransaction(Transaction t);
        void finishBlock(int blockId);
        set<Transaction>& getTransactions(int blockId);
        std::pair<char*, size_t> getRaw();
    protected:
        BlockChain & blockchain;
        HostManager & hosts;
        map<int, set<Transaction>> transactionQueue;
        vector<std::thread> syncThread;
        std::mutex lock;
        friend void mempool_sync(MemPool& mempool);
};