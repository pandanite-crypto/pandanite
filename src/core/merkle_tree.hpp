#ifndef MERKLE_TREE_HPP
#define MERKLE_TREE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "transaction.hpp"
#include "sha256.hpp"

using std::make_shared;
using std::shared_ptr;
using std::vector;
using std::unordered_map;

const SHA256Hash NULL_SHA256_HASH = SHA256Hash();

class HashTree {
public:
    shared_ptr<HashTree> parent;
        shared_ptr<HashTree> left;
            shared_ptr<HashTree> right;
                SHA256Hash hash;

                    HashTree(const SHA256Hash& hash);
                        ~HashTree();
                        };

                        class MerkleTree {
                        private:
                            shared_ptr<HashTree> root;
                                unordered_map<SHA256Hash, shared_ptr<HashTree>> fringeNodes;

                                public:
                                    MerkleTree();
                                        ~MerkleTree();

                                            void setItems(const vector<Transaction>& items);
                                                SHA256Hash getRootHash() const;
                                                    shared_ptr<HashTree> getMerkleProof(const Transaction& t) const;
                                                    };

                                                    std::ostream& operator<<(std::ostream& os, const HashTree& node);

                                                    #endif /* MERKLE_TREE_HPP */
                                                    