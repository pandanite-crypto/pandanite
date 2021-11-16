#pragma once
#include "block.hpp"
#include "constants.hpp"
#include "common.hpp"
#include "ledger.hpp"
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
    UNKNOWN_ERROR,
    SUCCESS
};

string executionStatusAsString(ExecutionStatus s);

class Executor {
    public:
        static void Rollback(Ledger& ledger, LedgerState& deltas);
        static void RollbackBlock(Block& curr, Ledger& ledger);
        static ExecutionStatus ExecuteBlock(Block& block, Ledger& ledger, LedgerState& deltas);
        static ExecutionStatus ExecuteTransaction(Ledger& ledger, Transaction t, LedgerState& deltas);
};