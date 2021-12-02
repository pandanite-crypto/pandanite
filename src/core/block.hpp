#pragma once
#include <vector>
#include <map>
#include "transaction.hpp"
#include "constants.hpp"
#include "common.hpp"
#include "crypto.hpp"
#include "merkle_tree.hpp"
using namespace std;

struct BlockHeader {
    uint32_t id;
    time_t timestamp;
    uint32_t difficulty;
    uint32_t numTransactions;
    SHA256Hash lastBlockHash;
    SHA256Hash merkleRoot;
    SHA256Hash nonce;
};

class Block {
    public:
        Block();
        Block(json data);
        Block(const Block& b);
        Block(const BlockHeader&b, vector<Transaction>& transactions);
        Block(std::pair<uint8_t*,size_t> buffer);
        BlockHeader serialize();
        json toJson();
        void addTransaction(Transaction t);
        void setNonce(SHA256Hash s);
        void setMerkleRoot(SHA256Hash s);
        void setTimestamp(time_t t);
        void setId(uint32_t id);
        void setDifficulty(uint8_t d);
        SHA256Hash getHash();
        SHA256Hash getNonce();
        SHA256Hash getMerkleRoot();
        SHA256Hash getLastBlockHash();
        void setLastBlockHash(SHA256Hash hash);
        time_t getTimestamp();
        uint8_t getDifficulty() const;
        vector<Transaction>& getTransactions();
        uint32_t getId();
        bool verifyNonce();
    // protected:
        uint32_t id;
        time_t timestamp;
        uint32_t difficulty;
        vector<Transaction> transactions;
        SHA256Hash merkleRoot;
        SHA256Hash lastBlockHash;
        SHA256Hash nonce;
    private:
        friend bool operator==(const Block& a, const Block& b);
};

bool operator==(const Block& a, const Block& b);
