#include "merkle_tree.hpp"
#include <queue>
#include <algorithm>
#include <iostream>
using namespace std;


MerkleTree::MerkleTree() {
}

MerkleTree::~MerkleTree() {
}

void MerkleTree::setItems(vector<Transaction>& items) {
    for(auto& t : items) {
        this->hashes.push_back(t.getHash());
    }

    std::sort(this->hashes.begin(), this->hashes.end(), [](const SHA256Hash& a, const SHA256Hash& b){
        return SHA256toString(a) > SHA256toString(b);
    });

    while(this->hashes.size() > 1) {
        if (this->hashes.size() % 2 != 0) {
            this->hashes.push_back(this->hashes[this->hashes.size() - 1]);
        }
        vector<SHA256Hash> next;
        for(int i = 0; i < this->hashes.size()/2; i++) {
            SHA256Hash a = this->hashes[2*i];
            SHA256Hash b = this->hashes[2*i + 1];
            next.push_back(concatHashes(a,b));
        }
        this->hashes = next;
    }
}

SHA256Hash MerkleTree::getRootHash() {
    return this->hashes[0];
}

