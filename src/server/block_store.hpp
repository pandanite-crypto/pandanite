#pragma once
#include <mutex>
#include "leveldb/db.h"
#include "../core/common.hpp"
#include "../core/block.hpp"
#include "data_store.hpp"

class BlockStore : public DataStore {
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
        bool hasBlockCount();

        vector<SHA256Hash> getTransactionsForWallet(PublicWalletAddress& wallet);
        void removeBlockWalletTransactions(Block& block);
    protected:
        vector<TransactionInfo> getBlockTransactions(BlockHeader& block);
};