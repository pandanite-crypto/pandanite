#pragma once

#include "../external/wasm-micro-runtime/core/iwasm/include/wasm_export.h"
#include "../core/block.hpp"
#include "../core/constants.hpp"
#include "../core/common.hpp"
#include "ledger.hpp"
#include "tx_store.hpp"
#include "state_store.hpp"
#include "program.hpp"
#include "executor.hpp"

using namespace std;

struct WasmEnvironment {
    wasm_module_t module;
    wasm_module_inst_t runtime;
    wasm_exec_env_t executor;
    uint32_t wasm_buffer;
    StateStore* state;
};

class WasmExecutor : public Executor {
    public:
        WasmExecutor(vector<uint8_t> byteCode);
        json getInfo(json args,  StateStore& store) override;
        Block getGenesis() const override;
        void rollback(Ledger& ledger, LedgerState& deltas, StateStore& store) override;
        void rollbackBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, BlockStore& blockStore, StateStore& store) override;
        ExecutionStatus executeBlock(Block& block, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas, StateStore& store) override;
        ExecutionStatus executeTransaction(Ledger& ledger, const Transaction t, LedgerState& deltas, StateStore& store) override;
        ExecutionStatus executeBlockWasm(Block& b, StateStore& store);
        WasmEnvironment* initWasm(StateStore& state) const;
        void cleanupWasm(WasmEnvironment* environment) const;
    protected:
        vector<uint8_t> byteCode;

        
};