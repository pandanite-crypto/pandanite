#include "merkle_tree.hpp"
#include <queue>
#include <algorithm>
#include <iostream>
using namespace std;



HashTree::HashTree(SHA256Hash hash) {
    parent = left = right = NULL;
    this->hash = hash;
}
HashTree::~HashTree() {
    if (this->left) delete this->left;
    if (this->right) delete this->right;
}

HashTree* getProof(HashTree* fringe, HashTree* previousNode=NULL) {
    HashTree* result = new HashTree(fringe->hash);
    if (previousNode != NULL) {
        if (fringe->left && fringe->left->hash != previousNode->hash) {
            result->left = new HashTree(fringe->left->hash);
            result->right = previousNode;
        } else if (fringe->right && fringe->right->hash != previousNode->hash) {
            result->right = new HashTree(fringe->right->hash);
            result->left = previousNode;
        }
    }
    if(fringe->parent) {
        return getProof(fringe->parent, result);
    } else {
        return result;
    }
}

MerkleTree::MerkleTree() {
    this->root = NULL;
}

MerkleTree::~MerkleTree() {
    delete this->root;
}

void MerkleTree::setItems(vector<Transaction>& items) {
    if (this->root) delete this->root;
    std::sort(items.begin(), items.end(), [](const Transaction & a, const Transaction & b) -> bool { 
        return SHA256toString(a.getHash()) > SHA256toString(b.getHash());
    });
    queue<HashTree*> q;
    for(auto item : items) {
        SHA256Hash h = item.getHash();
        this->fringeNodes[h] = new HashTree(h);
        q.push(this->fringeNodes[h]);
    }

    if (q.size()%2 == 1) {
        auto repeat = new HashTree(q.back()->hash);
        q.push(repeat);
    }
    
    while(q.size()>1) {
        HashTree* a = q.front();
        q.pop();
        HashTree* b = q.front();
        q.pop();
        HashTree* root = new HashTree(NULL_SHA256_HASH);
        root->left = a;
        root->right = b;
        a->parent = root;
        b->parent = root;
        root->hash = concatHashes(a->hash, b->hash);
        q.push(root);
    }
    this->root = q.front();
}

SHA256Hash MerkleTree::getRootHash() {
    return this->root->hash;
}

HashTree* MerkleTree::getMerkleProof(Transaction t) {
    SHA256Hash hash = t.getHash();
    if (this->fringeNodes.find(hash) == this->fringeNodes.end()) return NULL;
    return getProof(this->fringeNodes[hash]);
}
