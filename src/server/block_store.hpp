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
        Block getBlock(uint32_t blockId)const;
        std::pair<uint8_t*, size_t> getRawData(uint32_t blockId) const;
        BlockHeader getBlockHeader(uint32_t blockId) const;
        void setBlock(Block& b);
        void setBlockCount(size_t count);
        size_t getBlockCount() const;
        void setTotalWork(Bigint work);
        Bigint getTotalWork() const;
        bool hasBlockCount();

        vector<SHA256Hash> getTransactionsForWallet(PublicWalletAddress& wallet) const;
        void removeBlockWalletTransactions(Block& block);
    protected:
        vector<TransactionInfo> getBlockTransactions(BlockHeader& block) const;
};
