#pragma once
#include "leveldb/db.h"
#include "common.hpp"
#include "block.hpp"

class BlockStore {
    public:
        BlockStore();
        void init(string path);
        bool hasBlock(int blockId);
        Block getBlock(int blockId);
        std::pair<char*, size_t> getRawData(int blockId);
        void setBlock(Block& b);
        void setBlockCount(size_t count);
        size_t getBlockCount();
        bool hasBlockCount();
        void deleteDB();
        void closeDB();
    protected:
        BlockHeader getBlockHeader(int blockId);
        vector<TransactionInfo> getBlockTransactions(BlockHeader& block);
        leveldb::DB *db;
        size_t numBlocks;
        string path;
};