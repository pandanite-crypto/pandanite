#pragma once
#include "block.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "executor.hpp"
#include "common.hpp"
#include "ledger.hpp"
#include <list>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
using namespace std;

class BlockChain {
    public:
        BlockChain();
        void sync(vector<string> hosts);
        void acquire();
        void release();
        void resetChain();
        bool isLoaded();
        Block getBlock(int idx);
        int getDifficulty();
        int getBlockCount();
        SHA256Hash getLastHash();
        Ledger& getLedger();
        ExecutionStatus addBlock(Block& block);
        ExecutionStatus verifyTransaction(Transaction& t);
    protected:
        vector<Block> chain;
        Ledger ledger;
        list<LedgerState> deltas;
        SHA256Hash lastHash;
        int difficulty;
        void updateDifficulty(Block& b);
        void updateKeyPool(Block& b);
        ExecutionStatus startChainSync();
        int targetBlockCount;
        std::mutex lock;
        vector<std::thread> syncThread;
        vector<string> hosts;
        friend void chain_sync(BlockChain& blockchain);
};