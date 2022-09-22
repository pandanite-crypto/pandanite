#include "common.hpp"


string executionStatusAsString(ExecutionStatus status) {
    switch(status) {
        case SENDER_DOES_NOT_EXIST:
            return "SENDER_DOES_NOT_EXIST";
        break;
        case BALANCE_TOO_LOW:
            return "BALANCE_TOO_LOW";
        break;
        case INVALID_SIGNATURE:
            return "INVALID_SIGNATURE";
        break;
        case INVALID_NONCE:
            return "INVALID_NONCE";
        break;
        case EXTRA_MINING_FEE:
            return "EXTRA_MINING_FEE";
        break;
        case INCORRECT_MINING_FEE:
            return "INCORRECT_MINING_FEE";
        break;
        case INVALID_BLOCK_ID:
            return "INVALID_BLOCK_ID";
        break;
        case NO_MINING_FEE:
            return "NO_MINING_FEE";
        break;
        case UNKNOWN_ERROR:
            return "UNKNOWN_ERROR";
        break;
        case INVALID_TRANSACTION_NONCE:
            return "INVALID_TRANSACTION_NONCE";
        break;
        case INVALID_DIFFICULTY:
            return "INVALID_DIFFICULTY";
        break;
        case INVALID_TRANSACTION_TIMESTAMP:
            return "INVALID_TRANSACTION_TIMESTAMP";
        break;
        case SUCCESS:
            return "SUCCESS";
        break;
        case QUEUE_FULL:
            return "QUEUE_FULL";
        break;
        case EXPIRED_TRANSACTION:
            return "EXPIRED_TRANSACTION";
        break;
        case ALREADY_IN_QUEUE:
            return "ALREADY_IN_QUEUE";
        break;
        case BLOCK_ID_TOO_LARGE:
            return "BLOCK_ID_TOO_LARGE";
        break;
        case INVALID_MERKLE_ROOT:
            return "INVALID_MERKLE_ROOT";
        break;
        case INVALID_LASTBLOCK_HASH:
            return "INVALID_LASTBLOCK_HASH";
        break;
        case BLOCK_TIMESTAMP_TOO_OLD:
            return "BLOCK_TIMESTAMP_TOO_OLD";
        break;
        case HEADER_HASH_INVALID:
            return "HEADER_HASH_INVALID";
        break;
        case INVALID_TRANSACTION_COUNT:
            return "INVALID_TRANSACTION_COUNT";
        case BLOCK_TIMESTAMP_IN_FUTURE:
            return "BLOCK_TIMESTAMP_IN_FUTURE";
        break;
        case WALLET_SIGNATURE_MISMATCH:
            return "WALLET_SIGNATURE_MISMATCH";
        break;
        case TRANSACTION_FEE_TOO_LOW:
            return "TRANSACTION_FEE_TOO_LOW";
        break;
        case IS_SYNCING:
            return "IS_SYNCING";
        break;
        case UNSUPPORTED_CHAIN:
            return "UNSUPPORTED_CHAIN";
        break;
        case ALREADY_HAS_PROGRAM:
            return "ALREADY_HAS_PROGRAM";
        break;
        case WALLET_LOCKED:
            return "WALLET_LOCKED";
        break;
        case WASM_ERROR:
            return "WASM_ERROR";
        break;
    }
}