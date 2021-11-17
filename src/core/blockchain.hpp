#pragma once
#include "block.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "executor.hpp"
#include "common.hpp"
#include "block_store.hpp"
#include "ledger.hpp"
#include "host_manager.hpp"
#include <list>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
using namespace std;

class BlockChain {
    public:
        BlockChain(HostManager& hosts);
        void sync();
        void acquire();
        void release();
        void resetChain();
        Block getBlock(int idx);
        int getDifficulty();
        int getBlockCount();
        SHA256Hash getLastHash();
        Ledger& getLedger();
        ExecutionStatus addBlock(Block& block);
        ExecutionStatus verifyTransaction(Transaction& t);
    protected:
        HostManager& hosts;
        int numBlocks;
        BlockStore blockStore;
        Ledger ledger;
        list<LedgerState> deltas;
        SHA256Hash lastHash;
        int difficulty;
        void distributeTaxes(Block& block);
        void updateDifficulty(Block& b);
        ExecutionStatus startChainSync();
        int targetBlockCount;
        std::mutex lock;
        vector<std::thread> syncThread;
        friend void chain_sync(BlockChain& blockchain);
};