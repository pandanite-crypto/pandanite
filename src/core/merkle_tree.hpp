#ifndef MERKLE_TREE_HPP
#define MERKLE_TREE_HPP

#include <memory>
#include <unordered_map>
#include <vector>
#include "transaction.hpp"
#include "sha256_utils.hpp"

class HashTree {
public:
    HashTree(SHA256Hash hash);
    ~HashTree();

    std::shared_ptr<HashTree> parent;
    std::shared_ptr<HashTree> left;
    std::shared_ptr<HashTree> right;
    SHA256Hash hash;
};

class MerkleTree {
public:
    MerkleTree();
    ~MerkleTree();

    void setItems(const std::vector<Transaction>& items);
    SHA256Hash getRootHash() const;
    std::shared_ptr<HashTree> getMerkleProof(const Transaction& t) const;

private:
    std::shared_ptr<HashTree> root;
    std::unordered_map<SHA256Hash, std::shared_ptr<HashTree>, SHA256Hash::Hasher> fringeNodes;

    std::shared_ptr<HashTree> getProof(std::shared_ptr<HashTree> fringe, std::shared_ptr<HashTree> previousNode = nullptr) const;
};

#endif // MERKLE_TREE_HPP
