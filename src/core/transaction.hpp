#pragma once
#include "constants.hpp"
#include "common.hpp"
#include "crypto.hpp"
#include <optional>
using namespace std;

#define ADDRESS_CHAR_COUNT 53
#define SIGNATURE_CHAR_COUNT 78

struct TransactionInfo {
    int blockId;
    char signature[64];
#ifdef SECP256K1
    char signingKey[64];
#else
    char signingKey[32];
#endif
    time_t timestamp;
    char nonce[TRANSACTION_NONCE_SIZE];
    PublicWalletAddress to;
    PublicWalletAddress from;
    TransactionAmount amount;
    TransactionAmount fee;
    bool isTransactionFee;
};

class Transaction {
    public:
        Transaction(json t);
        Transaction();
        Transaction(const Transaction & t);
        Transaction(PublicWalletAddress to, int blockId);
        Transaction(PublicWalletAddress from, PublicWalletAddress to, TransactionAmount amount, int blockId, PublicKey signingKey, TransactionAmount fee=0);
        Transaction(const TransactionInfo& t);
        TransactionInfo serialize();
        json toJson();
        void setBlockId(int id);
        int getBlockId() const;
        void setNonce(string n);
        string getNonce() const;
        void sign(PublicKey pubKey, PrivateKey signingKey);
        void setTransactionFee(TransactionAmount amount);
        TransactionAmount getTransactionFee() const;
        void setAmount(TransactionAmount amt);
        PublicWalletAddress fromWallet() const;
        PublicWalletAddress toWallet() const;
        TransactionAmount getAmount() const;
        void setTimestamp(time_t t);
        time_t getTimestamp();
        SHA256Hash getHash() const;
        SHA256Hash hashContents() const;
        TransactionSignature getSignature() const;
        bool signatureValid() const;
        bool isFee() const;
    protected:
        int blockId;
        string nonce;
        TransactionSignature signature;
        PublicKey signingKey;
        time_t timestamp;
        PublicWalletAddress to;
        PublicWalletAddress from;
        TransactionAmount amount;
        TransactionAmount fee;
        bool isTransactionFee;
        friend bool operator==(const Transaction& a, const Transaction& b);
        friend bool operator<(const Transaction& a, const Transaction& b);
};

bool operator==(const Transaction& a, const Transaction& b);

