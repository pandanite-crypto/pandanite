#pragma once
#include <list>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include "block.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "executor.hpp"
#include "common.hpp"
#include "block_store.hpp"
#include "ledger.hpp"
#include "host_manager.hpp"
using namespace std;

class BlockChain {
    public:
        BlockChain(HostManager& hosts, bool deleteDB=false);
        void sync();
        void acquire();
        void release();
        void resetChain();
        void setTaxRate(double rate);
        Block getBlock(int blockId);
        int getDifficulty();
        int getBlockCount();
        SHA256Hash getLastHash();
        Ledger& getLedger();
        ExecutionStatus addBlock(Block& block);
        ExecutionStatus verifyTransaction(const Transaction& t);
        std::pair<char*, size_t> getRaw(int blockId);
        void deleteDB();
    protected:
        HostManager& hosts;
        int numBlocks;
        BlockStore blockStore;
        Ledger ledger;
        SHA256Hash lastHash;
        int difficulty;
        double taxRate;
        void popBlock();
        void distributeTaxes(Block& block);
        void updateDifficulty(Block& b);
        ExecutionStatus startChainSync();
        int targetBlockCount;
        std::mutex lock;
        vector<std::thread> syncThread;
        friend void chain_sync(BlockChain& blockchain);
};