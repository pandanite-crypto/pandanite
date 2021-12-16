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
    this->lastBlockHash = NULL_SHA256_HASH;
}

Block::Block(const Block& b) {
    this->nonce = b.nonce;
    this->id = b.id;
    this->difficulty = b.difficulty;
    this->timestamp = b.timestamp;
    this->merkleRoot = b.merkleRoot;
    this->lastBlockHash = b.lastBlockHash;
    this->transactions = vector<Transaction>();
    for(auto t : b.transactions) {
        this->transactions.push_back(t);
    }
}

Block::Block(json block) {
    this->nonce = stringToSHA256(block["nonce"]);
    this->merkleRoot = stringToSHA256(block["merkleRoot"]);
    this->lastBlockHash = stringToSHA256(block["lastBlockHash"]);
    this->id = block["id"];
    this->difficulty = block["difficulty"];
    this->timestamp = stringToTime(block["timestamp"]);
    for(auto t : block["transactions"]) {
        Transaction curr = Transaction(t);
        this->transactions.push_back(curr);
    }
}

Block::Block(std::pair<uint8_t*,size_t> buffer) {
    BlockHeader b = *((BlockHeader*)buffer.first);
    uint8_t * transactionPtr = buffer.first + sizeof(BlockHeader);

    this->id = b.id;
    this->timestamp = b.timestamp;
    this->difficulty = b.difficulty;
    this->nonce= b.nonce;
    this->merkleRoot = b.merkleRoot;
    this->lastBlockHash = b.lastBlockHash;

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
    this->lastBlockHash = b.lastBlockHash;
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
    b.lastBlockHash = this->lastBlockHash;
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
    result["lastBlockHash"] = SHA256toString(this->lastBlockHash);
    
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

uint32_t Block::getId() {
    return this->id;
}

void Block::setId(uint32_t id) {
    this->id = id;
}


void Block::addTransaction(Transaction t) {
    this->transactions.push_back(t);
}

void Block::setNonce(SHA256Hash s) {
    this->nonce = s;
}

SHA256Hash Block::getNonce() {
    return this->nonce;
}

void Block::setMerkleRoot(SHA256Hash s) {
    this->merkleRoot = s;
}

SHA256Hash Block::getMerkleRoot() {
    return this->merkleRoot;
}

vector<Transaction>& Block::getTransactions() {
    return this->transactions;
}

bool Block::verifyNonce() {
    if (this->lastBlockHash == NULL_SHA256_HASH) return true;
    SHA256Hash target = this->getHash();
    return verifyHash(target, this->nonce, this->difficulty);
}

void Block::setDifficulty(uint8_t d) {
    this->difficulty = d;
}

uint8_t Block::getDifficulty() const {
    return this->difficulty;
}

SHA256Hash Block::getLastBlockHash() {
    return this->lastBlockHash;
}

void Block::setLastBlockHash(SHA256Hash hash) {
    this->lastBlockHash = hash;
}

SHA256Hash Block::getHash() {
    SHA256Hash ret;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, (unsigned char*)this->merkleRoot.data(), this->merkleRoot.size());
    SHA256_Update(&sha256, (unsigned char*)this->lastBlockHash.data(), this->lastBlockHash.size());
    SHA256_Update(&sha256, (unsigned char*)&this->difficulty, sizeof(int));
    SHA256_Update(&sha256, (unsigned char*)&this->timestamp, sizeof(time_t));
    SHA256_Final(ret.data(), &sha256);
    return ret;
}

bool operator==(const Block& a, const Block& b) {
    if(b.id != a.id) return false;
    if(b.nonce != a.nonce) return false;
    if(b.timestamp != a.timestamp) return false;
    if(b.lastBlockHash != a.lastBlockHash) return false;
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