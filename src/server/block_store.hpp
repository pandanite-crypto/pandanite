#pragma once
#include "leveldb/db.h"
#include "../core/common.hpp"
#include "../core/block.hpp"

class BlockStore {
    public:
        BlockStore();
        void init(string path);
        bool hasBlock(uint32_t blockId);
        Block getBlock(uint32_t blockId);
        std::pair<uint8_t*, size_t> getRawData(uint32_t blockId);
<<<<<<< HEAD
=======
        std::pair<uint8_t*, size_t> getBlockHeaders(uint32_t start, uint32_t end);
>>>>>>> e10563b... basic chain working
        BlockHeader getBlockHeader(uint32_t blockId);
        void setBlock(Block& b);
        void setBlockCount(size_t count);
        size_t getBlockCount();
        void setTotalWork(uint64_t work);
        uint64_t getTotalWork();
        bool hasBlockCount();
        void deleteDB();
        void closeDB();
    protected:
        vector<TransactionInfo> getBlockTransactions(BlockHeader& block);
        leveldb::DB *db;
        size_t numBlocks;
        string path;
};