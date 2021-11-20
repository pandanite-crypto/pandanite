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
// Should take in and synchornize serialized tx's
//https://stackoverflow.com/questions/59045074/c-how-can-i-send-binary-dataprotobuf-data-using-http-post-request-through-a

class MemPool {
    public:
        MemPool(HostManager& h, BlockChain &b);
        void sync();
        ExecutionStatus addTransaction(Transaction t);
        void finishBlock(int blockId);
        set<Transaction>& getTransactions(int blockId);
    protected:
        BlockChain & blockchain;
        HostManager & hosts;
        map<int, set<Transaction>> transactionQueue;
        vector<std::thread> syncThread;
        std::mutex lock;
        friend void mempool_sync(MemPool& mempool);
};