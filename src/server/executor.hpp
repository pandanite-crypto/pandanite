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
        virtual json getInfo(json args,  StateStore& store);
        virtual Block getGenesis() const;
        virtual void rollback(Ledger& ledger, LedgerState& deltas, StateStore& store);
        virtual void rollbackBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, BlockStore& blockStore, StateStore& store);
        virtual ExecutionStatus executeBlock(Block& block, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas, StateStore& store);
        virtual ExecutionStatus executeTransaction(Ledger& ledger, const Transaction t, LedgerState& deltas, StateStore& store);
        virtual int updateDifficulty(int initialDifficulty, uint64_t numBlocks, const Program& program, StateStore& store);
        virtual TransactionAmount getMiningFee(uint64_t blockId, StateStore& store);
};