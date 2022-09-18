#pragma once
#include "../core/common.hpp"
#include "../core/crypto.hpp"
#include "../core/host_manager.hpp"
#include "ledger.hpp"
#include "tx_store.hpp"
#include "block_store.hpp"
#include "program.hpp"
#include "executor.hpp"

using namespace std;

class Program {
    public:
        Program(vector<uint8_t>& byteCode, HostManager& hosts);
        Program(json obj, HostManager& hosts);
        Program(HostManager& hosts);

        // get program info
        ProgramID getId() const;
        vector<uint8_t> getByteCode() const;
        json toJson() const;

        // reset state
        void clearState();
        void closeDB();
        void deleteDB();

        // data access methods
        Block getGenesis() const;
        uint32_t blockForTransactionId(SHA256Hash txid) const;
        Block getBlock(uint64_t blockId) const;
        std::pair<uint8_t*, size_t> getRawBlock(uint64_t blockId) const;
        BlockHeader getBlockHeader(uint64_t blockId) const;
        TransactionAmount getWalletValue(PublicWalletAddress& wallet) const;
        bool hasTransaction(Transaction& t) const;
        bool hasWalletProgram(PublicWalletAddress& wallet) const;
        Program getWalletProgram(PublicWalletAddress& wallet) const;

        // update methods
        ExecutionStatus executeBlock(Block& block, LedgerState& deltasFromBlock);
        ExecutionStatus executeTransaction(const Transaction& t, LedgerState& deltas);
        void rollback(LedgerState& deltas);
        void rollbackBlock(Block& block);
        void setTotalWork(Bigint total);
        void setBlockCount(uint64_t count);
    protected:
        void removeBlockWalletTransactions(Block& b);
        HostManager& hosts;
        std::shared_ptr<Executor> executor;
        ProgramID id;
        BlockStore blockStore;
        Ledger ledger;
        TransactionStore txdb;
        vector<uint8_t> byteCode;
        
        friend bool operator==(const Program& a, const Program& b);
};

bool operator==(const Program& a, const Program& b);