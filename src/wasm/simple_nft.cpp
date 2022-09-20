
#include "../core/block.hpp"
#include "../core/transaction.hpp"
#include "../core/helpers.hpp"
using namespace std;

Block genesis() {
    Block genesis;
    genesis.setTimestamp(0);
    // add initial coins to a wallet:
    Transaction mintTransaction = Transaction::createGenesisTransaction(NULL_SHA256_HASH, 1);
    genesis.addTransaction(mintTransaction);
    return genesis;
}


TransactionAmount getMiningFee(uint64_t blockId) {
    return 0;
}

int updateDifficulty(int initialDifficulty, uint64_t numBlocks, const Program& program) {
    return initialDifficulty;
}

void rollback(Ledger& ledger, LedgerState& deltas) {

}

void rollbackBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, BlockStore& blockStore) {

}

ExecutionStatus executeTransaction(Ledger& ledger, const Transaction t,  LedgerState& deltas) {

}

ExecutionStatus executeBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas) {

}

