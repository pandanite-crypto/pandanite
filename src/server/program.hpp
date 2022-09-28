#pragma once
#include "../core/common.hpp"
#include "../core/crypto.hpp"
#include "../shared/host_manager.hpp"
#include "ledger.hpp"
#include "tx_store.hpp"
#include "block_store.hpp"
#include "program.hpp"

using namespace std;

class Executor;

class Program {
    public:
        Program(vector<uint8_t>& byteCode);
        Program(json obj);
        Program();

        // get program info
        ProgramID getId() const;
        vector<uint8_t> getByteCode() const;
        json toJson() const;

        // reset state
        void clearState();
        void closeDB();
        void deleteDB();

        // data access methods
        uint64_t getBlockCount() const;
        Bigint getTotalWork() const;
        bool hasBlockCount() const;
        Block getGenesis() const;
        uint64_t blockForTransaction(const Transaction& t) const;
        vector<SHA256Hash> getTransactionsForWallet(const PublicWalletAddress& wallet) const;
        uint32_t blockForTransactionId(SHA256Hash txid) const;
        Block getBlock(uint64_t blockId) const;
        std::pair<uint8_t*, size_t> getRawBlock(uint64_t blockId) const;
        BlockHeader getBlockHeader(uint64_t blockId) const;
        TransactionAmount getWalletValue(PublicWalletAddress& wallet) const;
        SHA256Hash getLastHash() const;
        int getDifficulty() const;
        TransactionAmount getCurrentMiningFee() const;
        bool hasTransaction(Transaction& t) const;
        bool hasWalletProgram(PublicWalletAddress& wallet) const;
        ProgramID getWalletProgram(PublicWalletAddress& wallet) const;

        // update methods
        ExecutionStatus executeBlock(Block& block);
        ExecutionStatus executeTransaction(const Transaction& t, LedgerState& deltas);
        void rollback(LedgerState& deltas);
        void rollbackBlock(Block& block);
        void setTotalWork(Bigint total);
        void setBlockCount(uint64_t count);
        void deleteData();
    protected:
        void removeBlockWalletTransactions(Block& b);
        std::shared_ptr<Executor> executor;
        ProgramID id;
        BlockStore blockStore;
        Ledger ledger;
        TransactionStore txdb;
        vector<uint8_t> byteCode;
        int numBlocks;
        Bigint totalWork;
        SHA256Hash lastHash;
        int difficulty;
        friend bool operator==(const Program& a, const Program& b);
};

bool operator==(const Program& a, const Program& b);