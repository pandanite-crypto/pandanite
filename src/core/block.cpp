#include "block.hpp"
#include "helpers.hpp"
#include "crypto.hpp"
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
    this->hasMerkleTree = false;
}

Block::Block(const Block& b) {
    this->nonce = b.nonce;
    this->id = b.id;
    this->difficulty = b.difficulty;
    this->timestamp = b.timestamp;
    this->transactions = vector<Transaction>();
    for(auto t : b.transactions) {
        this->transactions.push_back(t);
    }
    this->computeMerkleTree();
}

Block::Block(json block) {
    this->nonce = stringToSHA256(block["nonce"]);
    this->id = block["id"];
    this->difficulty = block["difficulty"];
    this->timestamp = stringToTime(block["timestamp"]);
    for(auto t : block["transactions"]) {
        Transaction curr = Transaction(t);
        this->transactions.push_back(curr);
    }
    this->computeMerkleTree();
}

Block::Block(const BlockHeader&b, vector<Transaction>& transactions) {
    this->id = b.id;
    this->timestamp = b.timestamp;
    this->difficulty = b.difficulty;
    this->nonce= b.nonce;
    for(auto t : transactions) {
        this->addTransaction(t);
    }
    if (!this->hasMerkleTree) {
        this->computeMerkleTree();
    }
}

BlockHeader Block::serialize() {
    if (!this->hasMerkleTree) {
        this->computeMerkleTree();
    }
    BlockHeader b;
    b.id = this->id;
    b.timestamp = this->timestamp;
    b.difficulty = this->difficulty;
    b.numTransactions = this->transactions.size();
    b.merkleRoot = this->merkleTree.getRootHash();
    b.nonce = this->nonce;
    return b;
}

json Block::toJson() {
    if (!this->hasMerkleTree) {
        this->computeMerkleTree();
    }
    json result;
    result["id"] = this->id;
    result["difficulty"] = this->difficulty;
    result["nonce"] = SHA256toString(this->nonce);
    result["merkleRoot"] = SHA256toString(this->merkleTree.getRootHash());
    result["timestamp"] = timeToString(this->timestamp);
    result["transactions"] = json::array();
    
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

void Block::addTransaction(Transaction t) {
    if (t.getBlockId() != this->id) throw std::runtime_error("transactionID does not match blockID");
    this->transactions.push_back(t);
    this->hasMerkleTree = false;
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

void Block::computeMerkleTree() {
    this->hasMerkleTree = true;
    this->merkleTree.setItems(this->transactions);
}

void Block::freeMerkleTree() {
    this->hasMerkleTree = false;
    vector<Transaction> empty;
    this->merkleTree.setItems(empty);
}

SHA256Hash Block::getHash(SHA256Hash previousHash) {
    if (!this->hasMerkleTree) this->computeMerkleTree();
    stringstream s;
    s<<timeToString(this->timestamp)<<":";
    s<<this->difficulty<<":";
    s<<SHA256toString(previousHash)<<":";  
    s<<SHA256toString(this->merkleTree.getRootHash())<<":";
    string str = s.str();
    SHA256Hash h = SHA256(s.str());
    return h;
}

bool operator==(const Block& a, const Block& b) {
    if(b.id != a.id) return false;
    if(b.nonce != a.nonce) return false;
    if(b.timestamp != a.timestamp) return false;
    if(b.difficulty != a.difficulty) return false;
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