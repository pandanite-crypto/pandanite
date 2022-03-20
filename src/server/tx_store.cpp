#include "tx_store.hpp"
#include <thread>


TransactionStore::TransactionStore() {
}

bool TransactionStore::hasTransaction(const Transaction &t) {
    SHA256Hash txHash = t.hashContents();
    return seen.find(txHash) != seen.end();
}

uint32_t TransactionStore::blockForTransaction(Transaction &t) {
    SHA256Hash txHash = t.hashContents();
    return seen[txHash];
}

void TransactionStore::insertTransaction(Transaction& t, uint32_t blockId) {
    SHA256Hash txHash = t.hashContents();
    seen[txHash] = blockId;
}

void TransactionStore::removeTransaction(Transaction& t) {
    SHA256Hash txHash = t.hashContents();
    seen.erase(txHash);
}
