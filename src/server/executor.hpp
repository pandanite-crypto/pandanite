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
    BLOCK_TIMESTAMP_IN_FUTURE,
    UNKNOWN_ERROR,
    QUEUE_FULL,
    HEADER_HASH_INVALID,
    EXPIRED_TRANSACTION,
    ALREADY_IN_QUEUE,
    BLOCK_ID_TOO_LARGE,
    INVALID_MERKLE_ROOT,
    INVALID_LASTBLOCK_HASH,
    INVALID_TRANSACTION_COUNT,
    WALLET_SIGNATURE_MISMATCH,
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