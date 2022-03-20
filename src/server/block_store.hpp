#pragma once
#include <map>
using namespace std;
#include "../core/common.hpp"
#include "../core/block.hpp"
#include "../core/crypto.hpp"

class BlockStore {
    public:
        BlockStore();
        bool hasBlock(uint32_t blockId);
        Block getBlock(uint32_t blockId);
        std::pair<uint8_t*, size_t> getRawData(uint32_t blockId);
        BlockHeader getBlockHeader(uint32_t blockId);
        void setBlock(Block& b);
        void setBlockCount(size_t count);
        size_t getBlockCount();
        void setTotalWork(Bigint work);
        Bigint getTotalWork();
    protected:
        vector<TransactionInfo> getBlockTransactions(BlockHeader& block);
        map<uint32_t, Block> blocks;
        Bigint totalWork;
        uint32_t blockCount;
};