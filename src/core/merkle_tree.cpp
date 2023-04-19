#include <iostream>
#include <memory>
#include <queue>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "merkle_tree.hpp"
#include "transaction.hpp"
#include "sha256_utils.hpp"

using namespace std;

// HashTree constructor
HashTree::HashTree(SHA256Hash hash) : parent(nullptr), left(nullptr), right(nullptr), hash(hash) {
}

// HashTree destructor
HashTree::~HashTree() {
}

// MerkleTree constructor
MerkleTree::MerkleTree() : root(nullptr) {
}

// MerkleTree destructor
MerkleTree::~MerkleTree() {
    root = nullptr;
}

// Private member function to generate Merkle proof
shared_ptr<HashTree> MerkleTree::getProof(shared_ptr<HashTree> fringe, shared_ptr<HashTree> previousNode) const {
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

void MerkleTree::setItems(const vector<Transaction>& items) {
    std::sort(items.begin(), items.end(), [](const Transaction& a, const Transaction& b) -> bool { 
        return SHA256toString(a.getHash()) > SHA256toString(b.getHash());
    });

    queue<shared_ptr<HashTree>> q;

    for (const auto& item : items) {
        SHA256Hash h = item.getHash();
        fringeNodes[h] = make_shared<HashTree>(h);
        q.push(fringeNodes[h]);
    }

    if (q.size() % 2 == 1) {
        auto repeat = make_shared<HashTree>(q.back()->hash);
        q.push(repeat);
    }

    while (q.size() > 1) {
        shared_ptr<HashTree> a = q.front();
        q.pop();
        shared_ptr<HashTree> b = q.front();
        q.pop();
        shared_ptr<HashTree> node = make_shared<HashTree>(NULL_SHA256_HASH);
        node->left = a;
        node->right = b;
        a->parent = node;
        b->parent = node;
        node->hash = concatHashes(a->hash, b->hash);
        q.push(node);
    }

    root = q.front();
}

SHA256Hash MerkleTree::getRootHash() const {
    return root->hash;
}

shared_ptr<HashTree> MerkleTree::getMerkleProof(const Transaction& t) const {
    SHA256Hash hash = t.getHash();
    auto iterator = fringeNodes.find(hash);

    if (iterator == fringeNodes.end()) return nullptr;

    return getProof(iterator->second);
}
