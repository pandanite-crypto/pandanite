#pragma once

#include <set>
#include <map>
#include <thread>
#include <mutex>
#include <list>
#include <map>
#include "../shared/host_manager.hpp"
#include "../core/transaction.hpp"
#include "executor.hpp"
using namespace std;

class BlockChain;
class VirtualChain;

class MemPool {
    public:
        MemPool(HostManager& h, BlockChain& b);
        ~MemPool();
        void sync();
        ExecutionStatus addTransaction(Transaction t);
        void addProgram(ProgramID programId, std::shared_ptr<VirtualChain> program);
        void finishBlock(Block& block, ProgramID programId = NULL_SHA256_HASH);
        bool hasTransaction(Transaction t);
        size_t size();
        std::pair<char*, size_t> getRaw(ProgramID programId = NULL_SHA256_HASH);
        vector<Transaction> getTransactions();
    protected:
        bool shutdown;
        std::mutex shutdownLock;
        map<PublicWalletAddress,TransactionAmount> mempoolOutgoing;
        list<Transaction> toSend;
        BlockChain & blockchain;
        HostManager & hosts;
        map<ProgramID, std::shared_ptr<VirtualChain>> programs;
        set<Transaction> transactionQueue;
        map<ProgramID, set<Transaction>> programTransactions;
        vector<std::thread> syncThread;
        std::mutex lock;
        std::mutex sendLock;
        friend void mempool_sync(MemPool& mempool);
};
