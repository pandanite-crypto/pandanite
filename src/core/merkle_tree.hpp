#pragma once
#include "transaction.hpp"
#include "crypto.hpp"
#include <map>
#include <string>
#include <vector>
#include <memory>
using namespace std;

class HashTree {
    public:
        HashTree(SHA256Hash hash);
        ~HashTree();
        SHA256Hash hash;
        shared_ptr<HashTree> parent;
        shared_ptr<HashTree> left;
        shared_ptr<HashTree> right;
};
class MerkleTree {
    public:
        MerkleTree();
        void setItems(vector<Transaction>& items);
        ~MerkleTree();
        bool verifyMerkleProof(SHA256Hash rootHash, HashTree* proof);
        shared_ptr<HashTree> getMerkleProof(Transaction t);
        SHA256Hash getRootHash();
    protected:
        shared_ptr<HashTree> root;
        map<SHA256Hash, shared_ptr<HashTree>> fringeNodes;
};