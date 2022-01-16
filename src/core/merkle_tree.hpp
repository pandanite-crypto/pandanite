#pragma once
#include "transaction.hpp"
#include "crypto.hpp"
#include <map>
#include <string>
#include <vector>
#include <memory>
using namespace std;

class MerkleTree {
    public:
        MerkleTree();
        void setItems(vector<Transaction>& items);
        ~MerkleTree();
        SHA256Hash getRootHash();
    protected:
        vector<SHA256Hash> hashes;
};