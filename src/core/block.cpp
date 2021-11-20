#include "block.hpp"
#include "helpers.hpp"
#include "crypto.hpp"
#include "openssl/sha.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <ctime>
#include <cstring>
using namespace std;

Block::Block() {
    this->nonce = NULL_SHA256_HASH;
    this->id = 1;
    this->timestamp = getCurrentTime();
    this->difficulty = MIN_DIFFICULTY;
    this->merkleRoot = NULL_SHA256_HASH;
}

Block::Block(const Block& b) {
    this->nonce = b.nonce;
    this->id = b.id;
    this->difficulty = b.difficulty;
    this->timestamp = b.timestamp;
    this->merkleRoot = b.merkleRoot;
    this->transactions = vector<Transaction>();
    for(auto t : b.transactions) {
        this->transactions.push_back(t);
    }
}

Block::Block(json block) {
    this->nonce = stringToSHA256(block["nonce"]);
    this->merkleRoot = stringToSHA256(block["merkleRoot"]);
    this->id = block["id"];
    this->difficulty = block["difficulty"];
    this->timestamp = stringToTime(block["timestamp"]);
    for(auto t : block["transactions"]) {
        Transaction curr = Transaction(t);
        this->transactions.push_back(curr);
    }
}

Block::Block(std::pair<char*,size_t> buffer) {
    BlockHeader b = *((BlockHeader*)buffer.first);
    char * transactionPtr = buffer.first + sizeof(BlockHeader);

    this->id = b.id;
    this->timestamp = b.timestamp;
    this->difficulty = b.difficulty;
    this->nonce= b.nonce;
    this->merkleRoot = b.merkleRoot;

    for(int i = 0; i < b.numTransactions; i++) {
        TransactionInfo t = *((TransactionInfo*)transactionPtr);
        this->addTransaction(Transaction(t));
        transactionPtr += sizeof(TransactionInfo);
    }
}

Block::Block(const BlockHeader&b, vector<Transaction>& transactions) {
    this->id = b.id;
    this->timestamp = b.timestamp;
    this->difficulty = b.difficulty;
    this->nonce= b.nonce;
    this->merkleRoot = b.merkleRoot;
    for(auto t : transactions) {
        this->addTransaction(t);
    }
}

BlockHeader Block::serialize() {
    BlockHeader b;
    b.id = this->id;
    b.timestamp = this->timestamp;
    b.difficulty = this->difficulty;
    b.numTransactions = this->transactions.size();
    b.nonce = this->nonce;
    b.merkleRoot = this->merkleRoot;
    return b;
}

json Block::toJson() {
    json result;
    result["id"] = this->id;
    result["difficulty"] = this->difficulty;
    result["nonce"] = SHA256toString(this->nonce);
    result["timestamp"] = timeToString(this->timestamp);
    result["transactions"] = json::array();
    result["merkleRoot"] = SHA256toString(this->merkleRoot);
    
    for(auto t : this->transactions) {
        result["transactions"].push_back(t.toJson());
    }
    return result;
}


void Block::setTimestamp(time_t t) {
    this->timestamp = t;
}

time_t Block::getTimestamp() {
    return this->timestamp;
}

int Block::getId() {
    return this->id;
}

void Block::setId(int id) {
    this->id = id;
}

HashTree* Block::verifyTransaction(Transaction &t) {
    return NULL;
}

void Block::addTransaction(Transaction t) {
    if (t.getBlockId() != this->id) throw std::runtime_error("transactionID does not match blockID");
    this->transactions.push_back(t);
}

void Block::setNonce(SHA256Hash s) {
    this->nonce = s;
}

SHA256Hash Block::getNonce() {
    return this->nonce;
}

vector<Transaction>& Block::getTransactions() {
    return this->transactions;
}

bool Block::verifyNonce(SHA256Hash previousHash) {
    if (previousHash == NULL_SHA256_HASH) return true;
    SHA256Hash target = this->getHash(previousHash);
    return verifyHash(target, this->nonce, this->difficulty);
}

void Block::setDifficulty(int d) {
    this->difficulty = d;
}

int Block::getDifficulty() const {
    return this->difficulty;
}

SHA256Hash Block::getHash(SHA256Hash previousHash) {
    SHA256Hash ret;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, (unsigned char*)previousHash.data(), previousHash.size());
    SHA256_Update(&sha256, (unsigned char*)this->merkleRoot.data(), this->merkleRoot.size());
    SHA256_Update(&sha256, (unsigned char*)&this->difficulty, sizeof(int));
    SHA256_Update(&sha256, (unsigned char*)&this->timestamp, sizeof(time_t));
    SHA256_Final(ret.data(), &sha256);
    return ret;
}

bool operator==(const Block& a, const Block& b) {
    if(b.id != a.id) return false;
    if(b.nonce != a.nonce) return false;
    if(b.timestamp != a.timestamp) return false;
    if(b.difficulty != a.difficulty) return false;
    if(b.merkleRoot != a.merkleRoot) return false;
    // check transactions equal
    if (a.transactions.size() != b.transactions.size()) return false;
    for(int i =0; i < a.transactions.size(); i++) {
        if(a.transactions[i] == b.transactions[i]) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}