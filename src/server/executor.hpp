#pragma once
#include "../core/block.hpp"
#include "../core/constants.hpp"
#include "../core/common.hpp"
#include "ledger.hpp"
#include "tx_store.hpp"
#include "program.hpp"

using namespace std;

class Executor {
    public:
        Block getGenesis() const;
        void rollback(Ledger& ledger, LedgerState& deltas) const;
        void rollbackBlock(Block& curr, Ledger& ledger, TransactionStore & txdb) const;
        ExecutionStatus executeBlock(Block& block, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas) const;
        ExecutionStatus executeTransaction(Ledger& ledger, const Transaction t, LedgerState& deltas) const;
        int updateDifficulty(int initialDifficulty, uint64_t numBlocks, const Program& program) const;
        TransactionAmount getMiningFee(uint64_t blockId) const;
};