#pragma once
#include "block.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "executor.hpp"
#include "common.hpp"
#include <string>
#include <vector>
#include <map>
#include <mutex>
using namespace std;

class BlockChain {
    public:
        BlockChain();
        void sync(string host);
        void acquire();
        void release();
        void resetChain();
        bool isLoaded();
        Block getBlock(int idx);
        int getDifficulty();
        int getBlockCount();
        SHA256Hash getLastHash();
        LedgerState& getLedger();
        ExecutionStatus addBlock(Block& block);
        ExecutionStatus verifyTransaction(Transaction& t);
    protected:
        vector<Block> chain;
        LedgerState ledger;
        SHA256Hash lastHash;
        int difficulty;
        void updateDifficulty(Block& b);
        void updateKeyPool(Block& b);
        ExecutionStatus startChainSync();
        int targetBlockCount;
        std::mutex lock;
        vector<std::thread> syncThread;
        string host;
        friend void chain_sync(BlockChain& blockchain);
};