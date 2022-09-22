#pragma once
#include "../core/block.hpp"
#include "../core/constants.hpp"
#include "../core/common.hpp"
#include "ledger.hpp"
#include "tx_store.hpp"
#include "state_store.hpp"
#include "program.hpp"
#include "executor.hpp"

using namespace std;

class WasmExecutor : public Executor {
    public:
        WasmExecutor(vector<uint8_t> byteCode);
        Block getGenesis() const;
        void rollback(Ledger& ledger, LedgerState& deltas, StateStore& store) const;
        void rollbackBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, BlockStore& blockStore, StateStore& store) const;
        ExecutionStatus executeBlock(Block& block, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas, StateStore& store) const;
        ExecutionStatus executeTransaction(Ledger& ledger, const Transaction t, LedgerState& deltas, StateStore& store) const;
        int updateDifficulty(int initialDifficulty, uint64_t numBlocks, const Program& program, StateStore& store) const;
        TransactionAmount getMiningFee(uint64_t blockId, StateStore& store) const;
        ExecutionStatus executeBlockWasm(Block& b, StateStore& store) const;
    protected:
        vector<uint8_t> byteCode;
};