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
        BlockChain(HostManager& hosts, string ledgerPath="", string blockPath="");
        void sync();
        void acquire();
        void release();
        void resetChain();
        Block getBlock(uint32_t blockId);
        uint64_t getTotalWork();
        uint8_t getDifficulty();
        uint32_t getBlockCount();
        SHA256Hash getLastHash();
        Ledger& getLedger();
        ExecutionStatus addBlock(Block& block);
        ExecutionStatus verifyTransaction(const Transaction& t);
        std::pair<uint8_t*, size_t> getRaw(uint32_t blockId);
        std::pair<uint8_t*, size_t>  getBlockHeaders(uint32_t start, uint32_t end);
        void deleteDB();
    protected:
        HostManager& hosts;
        uint32_t numBlocks;
        uint64_t totalWork;
        BlockStore blockStore;
        Ledger ledger;
        SHA256Hash lastHash;
        uint8_t difficulty;
        void popBlock();
        void updateDifficulty(Block& b);
        ExecutionStatus startChainSync();
        int targetBlockCount;
        std::mutex lock;
        vector<std::thread> syncThread;
        friend void chain_sync(BlockChain& blockchain);
};