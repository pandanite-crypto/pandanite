#include "merkle_tree.hpp"
#include <queue>
#include <algorithm>
#include <iostream>
using namespace std;



HashTree::HashTree(SHA256Hash hash) {
    parent = left = right = nullptr;
    this->hash = hash;
}
HashTree::~HashTree() {
}

shared_ptr<HashTree> getProof(shared_ptr<HashTree> fringe, shared_ptr<HashTree> previousNode=NULL) {
    shared_ptr<HashTree> result = make_shared<HashTree>(fringe->hash);
    if (previousNode != NULL) {
        if (fringe->left && fringe->left != previousNode) {
            result->left = fringe->left;
            result->right = previousNode;
        } else if (fringe->right && fringe->right != previousNode) {
            result->right = fringe->right;
            result->left = previousNode;
        }
    }
    if(fringe->parent) {
        return getProof(fringe->parent, fringe);
    } else {
        return result;
    }
}

MerkleTree::MerkleTree() {
    this->root = nullptr;
}

MerkleTree::~MerkleTree() {
    this->root = nullptr;
}

void MerkleTree::setItems(vector<Transaction>& items) {
    std::sort(items.begin(), items.end(), [](const Transaction & a, const Transaction & b) -> bool { 
        return SHA256toString(a.getHash()) > SHA256toString(b.getHash());
    });
    queue<shared_ptr<HashTree>> q;
    for(auto item : items) {
        SHA256Hash h = item.getHash();
        this->fringeNodes[h] =  make_shared<HashTree>(h);
        q.push(this->fringeNodes[h]);
    }

    if (q.size()%2 == 1) {
        auto repeat = make_shared<HashTree>(q.back()->hash);
        q.push(repeat);
    }
    
    while(q.size()>1) {
        shared_ptr<HashTree> a = q.front();
        q.pop();
        shared_ptr<HashTree> b = q.front();
        q.pop();
        shared_ptr<HashTree> root = make_shared<HashTree>(NULL_SHA256_HASH);
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

shared_ptr<HashTree> MerkleTree::getMerkleProof(Transaction t) {
    SHA256Hash hash = t.getHash();
    if (this->fringeNodes.find(hash) == this->fringeNodes.end()) return NULL;
    return getProof(this->fringeNodes[hash]);
}
