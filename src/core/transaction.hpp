#pragma once
#include "constants.hpp"
#include "common.hpp"
#include "crypto.hpp"
#include <optional>
using namespace std;

#define ADDRESS_CHAR_COUNT 53
#define SIGNATURE_CHAR_COUNT 78


class Transaction {
    public:
        Transaction(json t);
        Transaction();
        Transaction(const Transaction & t);
        Transaction(PublicWalletAddress to, int blockId);
        Transaction(PublicWalletAddress from, PublicWalletAddress to, TransactionAmount amount, int blockId, PublicKey signingKey, TransactionAmount fee=0);
        json toJson();
        void setBlockId(int id);
        int getBlockId() const;
        void setNonce(string n);
        string getNonce() const;
        void sign(TransactionSignature signature);
        void setTransactionFee(TransactionAmount amount);
        TransactionAmount getTransactionFee() const;
        void setMinerWallet(PublicWalletAddress amount);
        PublicWalletAddress getMinerWallet() const;
        void setAmount(TransactionAmount amt);
        PublicWalletAddress fromWallet() const;
        PublicWalletAddress toWallet() const;
        TransactionAmount getAmount() const;
        void setTimestamp(time_t t);
        time_t getTimestamp();
        string toString() const;
        SHA256Hash getHash() const;
        TransactionSignature getSignature() const;
        bool signatureValid() const;
        bool isFee() const;
        bool hasMiner() const;
    protected:
        int blockId;
        string nonce;
        TransactionSignature signature;
        PublicKey signingKey;
        time_t timestamp;
        optional<PublicWalletAddress> miner;
        PublicWalletAddress to;
        PublicWalletAddress from;
        TransactionAmount amount;
        TransactionAmount fee;
        bool isTransactionFee;
        friend bool operator==(const Transaction& a, const Transaction& b);
};

bool operator==(const Transaction& a, const Transaction& b);

