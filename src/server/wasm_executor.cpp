#include "wasm_executor.hpp"

#include <map>
#include <optional>
#include <iostream>
#include <set>
#include <cmath>
#include "../core/block.hpp"
#include "../core/logger.hpp"
#include "../core/helpers.hpp"
#include "executor.hpp"
using namespace std;


WasmExecutor::WasmExecutor(vector<uint8_t> byteCode) {

}

Block WasmExecutor::getGenesis() const {
    // TODO: call wasm
    return Block();
}

TransactionAmount WasmExecutor::getMiningFee(uint64_t blockId) const {
    // TODO: call wasm
    return 0;
}

int WasmExecutor::updateDifficulty(int initialDifficulty, uint64_t numBlocks, const Program& program) const{
    // TODO: call wasm
    return 0;
}

void WasmExecutor::rollback(Ledger& ledger, LedgerState& deltas) const{
    // TODO: call wasm
}

void WasmExecutor::rollbackBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, BlockStore& blockStore) const{
    // TODO call wasm
}

ExecutionStatus WasmExecutor::executeBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas) const{
    // TODO call wasm
    return SUCCESS;
}

