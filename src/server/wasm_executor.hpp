#pragma once
#include "../core/block.hpp"
#include "../core/constants.hpp"
#include "../core/common.hpp"
#include "ledger.hpp"
#include "tx_store.hpp"
#include "program.hpp"
#include "executor.hpp"
#include "../external/eosio/vm/backend.hpp"

using namespace std;

struct example_host_methods {
   // example of a host "method"
   void print_name(const char* nm) { std::cout << "Name : " << nm << " " << field << "\n"; }
   // example of another type of host function
   static void* memset(void* ptr, int x, size_t n) { return ::memset(ptr, x, n); }
   std::string  field = "";
};

class WasmExecutor : public Executor {
    public:
        WasmExecutor(vector<uint8_t> byteCode);
        Block getGenesis() const;
        void rollback(Ledger& ledger, LedgerState& deltas) const;
        void rollbackBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, BlockStore& blockStore) const;
        ExecutionStatus executeBlock(Block& block, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas) const;
        ExecutionStatus executeTransaction(Ledger& ledger, const Transaction t, LedgerState& deltas) const;
        int updateDifficulty(int initialDifficulty, uint64_t numBlocks, const Program& program) const;
        TransactionAmount getMiningFee(uint64_t blockId) const;
    protected:
        std::shared_ptr<eosio::vm::backend<example_host_methods>> wasmRuntime;
};