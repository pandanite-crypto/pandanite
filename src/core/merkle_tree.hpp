#pragma once
#include "transaction.hpp"
#include "crypto.hpp"
#include <map>
#include <string>
#include <vector>
using namespace std;

class HashTree {
    public:
        HashTree(SHA256Hash hash);
        ~HashTree();
        SHA256Hash hash;
        HashTree* parent;
        HashTree* left;
        HashTree* right;
};
class MerkleTree {
    public:
        MerkleTree();
        void setItems(vector<Transaction>& items);
        ~MerkleTree();
        bool verifyMerkleProof(SHA256Hash rootHash, HashTree* proof);
        HashTree* getMerkleProof(Transaction t);
        SHA256Hash getRootHash();
    protected:
        HashTree* root;
        map<SHA256Hash, HashTree*> fringeNodes;
};