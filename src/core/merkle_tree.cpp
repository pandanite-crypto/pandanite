#include "merkle_tree.hpp"

#include <queue>

#include <algorithm>

#include <iostream>

HashTree::HashTree(const SHA256Hash& hash) : parent(nullptr), left(nullptr), right(nullptr), hash(hash) {}

HashTree::~HashTree() {}

shared_ptr<HashTree> getProof(shared_ptr<HashTree>& fringe, const shared_ptr<HashTree>& previousNode = nullptr) {

    shared_ptr<HashTree> result = make_shared<HashTree>(fringe->hash);

    if (previousNode != nullptr) {

        if (fringe->left && fringe->left != previousNode) {

            result->left = fringe->left;

            result->right = previousNode;

        } else if (fringe->right && fringe->right != previousNode) {

            result->right = fringe->right;

            result->left = previousNode;

        }

    }

    if (fringe->parent) {

        return getProof(fringe->parent, fringe);

    } else {

        return result;

    }

}

MerkleTree::MerkleTree() : root(nullptr) {}

MerkleTree::~MerkleTree() {

    root = nullptr;

}

void MerkleTree::setItems(const vector<Transaction>& items) {

    if (items.empty()) {

        return;

    }

    vector<Transaction> sortedItems = items;

    std::sort(sortedItems.begin(), sortedItems.end(), [](const Transaction& a, const Transaction& b) -> bool {

        return SHA256toString(a.getHash()) > SHA256toString(b.getHash());

    });

    queue<shared_ptr<HashTree>> q;

    for (const auto& item : sortedItems) {

        SHA256Hash h = item.getHash();

        fringeNodes[h] = make_shared<HashTree>(h);

        q.push(fringeNodes[h]);

    }

    if (q.size() % 2 == 1) {

        auto repeat = make_shared<HashTree>(q.back

        q.push(repeat);

    }

    while (q.size() > 1) {

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

    root = q.front();

}

SHA256Hash MerkleTree::getRootHash() const {

    if (root == nullptr) {

        return NULL_SHA256_HASH;

    }

    return root->hash;

}

shared_ptr<HashTree> MerkleTree::getMerkleProof(const Transaction& t) const {

    SHA256Hash hash = t.getHash();

    auto iterator = fringeNodes.find(hash);

    if (iterator == fringeNodes.end()) {

        return nullptr;

    }

    return getProof(iterator->second);

}












    
    
        






        

    
        
        
    
    
        
        
        
    
    
    
