#pragma once
#include <map>
#include <vector>
#include <string>
#include <array>
#include "../external/bigint/bigint.h"
using namespace std;
using namespace Dodecahedron;

typedef std::array<uint8_t, 25> PublicWalletAddress;
typedef uint64_t TransactionAmount;

typedef std::array<uint8_t,32> PublicKey;
typedef std::array<uint8_t,64> PrivateKey;
typedef std::array<uint8_t,64> TransactionSignature;


typedef map<PublicWalletAddress,TransactionAmount> LedgerState;
typedef std::array<uint8_t, 32> SHA256Hash;
typedef std::array<uint8_t, 20> RIPEMD160Hash;
typedef std::array<uint8_t, 128> TransactionData;
typedef SHA256Hash ProgramID;

#define NULL_SHA256_HASH SHA256Hash({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0})
#define NULL_KEY SHA256Hash({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0})
#define NULL_ADDRESS PublicWalletAddress({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0})


struct BlockHeader {
    uint32_t id;
    uint64_t timestamp; 
    uint32_t difficulty;
    uint32_t numTransactions;
    SHA256Hash lastBlockHash;
    SHA256Hash merkleRoot;
    SHA256Hash nonce;
};

struct TransactionInfo {
    char signature[64];
    char signingKey[32];
    uint64_t nonce;
    PublicWalletAddress to;
    PublicWalletAddress from;
    TransactionAmount amount;
    TransactionAmount fee;
    bool isTransactionFee;
    ProgramID programId;
    TransactionData data;
};

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
    TRANSACTION_FEE_TOO_LOW,
    WALLET_SIGNATURE_MISMATCH,
    IS_SYNCING,
    UNSUPPORTED_CHAIN,
    ALREADY_HAS_PROGRAM,
    WALLET_LOCKED,
    SUCCESS,
    WASM_ERROR
};