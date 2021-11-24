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
    int id;
    time_t timestamp;
    int difficulty;
    int numTransactions;
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
        Block(std::pair<char*,size_t> buffer);
        BlockHeader serialize();
        json toJson();
        void addTransaction(Transaction t);
        void setNonce(SHA256Hash s);
        void setMerkleRoot(SHA256Hash s);
        void setTimestamp(time_t t);
        void setId(int id);
        void setDifficulty(int d);
        SHA256Hash getHash();
        SHA256Hash getNonce();
        SHA256Hash getMerkleRoot();
        SHA256Hash getLastBlockHash();
        void setLastBlockHash(SHA256Hash hash);
        time_t getTimestamp();
        int getDifficulty() const;
        vector<Transaction>& getTransactions();
        int getId();
        bool verifyNonce();
    // protected:
        int id;
        time_t timestamp;
        int difficulty;
        vector<Transaction> transactions;
        SHA256Hash merkleRoot;
        SHA256Hash lastBlockHash;
        SHA256Hash nonce;
    private:
        friend bool operator==(const Block& a, const Block& b);
};

bool operator==(const Block& a, const Block& b);
