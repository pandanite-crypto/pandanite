#pragma once
#include "../core/block.hpp"
#include "../core/constants.hpp"
#include "../core/common.hpp"
#include "ledger.hpp"
#include "tx_store.hpp"
using namespace std;

enum ExecutionStatus {
    SENDER_DOES_NOT_EXIST,
    BALANCE_TOO_LOW,
    INVALID_SIGNATURE,
    INVALID_NONCE,
    EXTRA_MINING_FEE,
    INCORRECT_MINING_FEE,
    INVALID_BLOCK_ID,
    NO_MINING_FEE,
    INVALID_DIFFICULTY,
    INVALID_TRANSACTION_NONCE,
    INVALID_TRANSACTION_TIMESTAMP,
    BLOCK_TIMESTAMP_TOO_OLD,
    UNKNOWN_ERROR,
    QUEUE_FULL,
    EXPIRED_TRANSACTION,
    BLOCK_ID_TOO_LARGE,
    INVALID_MERKLE_ROOT,
    INVALID_LASTBLOCK_HASH,
    SUCCESS
};

string executionStatusAsString(ExecutionStatus s);

class Executor {
    public:
        static void Rollback(Ledger& ledger, LedgerState& deltas);
        static void RollbackBlock(Block& curr, Ledger& ledger, TransactionStore & txdb);
        static ExecutionStatus ExecuteBlock(Block& block, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas, TransactionAmount miningFee);
        static ExecutionStatus ExecuteTransaction(Ledger& ledger, Transaction t, LedgerState& deltas);
};