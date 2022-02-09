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

class MemPool;

class BlockChain {
    public:
        BlockChain(HostManager& hosts, string ledgerPath="", string blockPath="", string txdbPath="");
        void sync();
        void acquire();
        void release();
        Block getBlock(uint32_t blockId);
        Bigint getTotalWork();
        uint8_t getDifficulty();
        uint32_t getBlockCount();
        uint32_t getCurrentMiningFee();
        SHA256Hash getLastHash();
        Ledger& getLedger();
        uint32_t findBlockForTransaction(Transaction &t);
        ExecutionStatus addBlock(Block& block);
        ExecutionStatus verifyTransaction(const Transaction& t);
        std::pair<uint8_t*, size_t> getRaw(uint32_t blockId);
        BlockHeader getBlockHeader(uint32_t blockId);
        TransactionAmount getWalletValue(PublicWalletAddress addr);
        void setMemPool(MemPool * memPool);
        void initChain();
        void resetChain();
        void popBlock();
        void deleteDB();
        void closeDB();
    protected:
        HostManager& hosts;
        MemPool * memPool;
        int numBlocks;
        Bigint totalWork;
        BlockStore blockStore;
        Ledger ledger;
        TransactionStore txdb;
        SHA256Hash lastHash;
        int difficulty;
        void updateDifficulty();
        ExecutionStatus startChainSync();
        int targetBlockCount;
        std::mutex lock;
        vector<std::thread> syncThread;
        map<int,Block> specialBlocks;
        map<int,SHA256Hash> checkpoints;
        friend void chain_sync(BlockChain& blockchain);
};