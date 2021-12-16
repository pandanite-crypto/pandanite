#pragma once
#include <list>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include "../core/block.hpp"
#include "../core/api.hpp"
#include "../core/constants.hpp"
#include "../core/common.hpp"
#include "../core/host_manager.hpp"
#include "executor.hpp"
#include "block_store.hpp"
#include "ledger.hpp"
#include "tx_store.hpp"

using namespace std;

class BlockChain {
    public:
        BlockChain(HostManager& hosts, string ledgerPath="", string blockPath="", string txdbPath="");
        void sync();
        void acquire();
        void release();
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
        void initChain();
        void resetChain();
        void popBlock();
        void deleteDB();
        void closeDB();
    protected:
        HostManager& hosts;
        int numBlocks;
        uint64_t totalWork;
        BlockStore blockStore;
        Ledger ledger;
        TransactionStore txdb;
        SHA256Hash lastHash;
        int difficulty;
        void updateDifficulty(Block& b);
        ExecutionStatus startChainSync();
        int targetBlockCount;
        std::mutex lock;
        vector<std::thread> syncThread;
        friend void chain_sync(BlockChain& blockchain);
};