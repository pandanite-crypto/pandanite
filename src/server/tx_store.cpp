#include "tx_store.hpp"
#include <thread>


TransactionStore::TransactionStore() {
}

bool TransactionStore::hasTransaction(const Transaction &t) {
    SHA256Hash txHash = t.hashContents();
    leveldb::Slice key = leveldb::Slice((const char*) txHash.data(), txHash.size());
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    return (status.ok());
}

uint32_t TransactionStore::blockForTransaction(Transaction &t) {
    SHA256Hash txHash = t.hashContents();
    leveldb::Slice key = leveldb::Slice((const char*) txHash.data(), txHash.size());
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    if (!status.ok()) throw std::runtime_error("Could not find specified transaction");
    uint32_t val = *((uint32_t*)value.c_str());
    return val;
}

void TransactionStore::insertTransaction(Transaction& t, uint32_t blockId) {
    SHA256Hash txHash = t.hashContents();
    leveldb::Slice key = leveldb::Slice((const char*) txHash.data(), txHash.size());
    uint32_t dummy = blockId;
    leveldb::Slice slice = leveldb::Slice((const char*)&dummy, sizeof(uint32_t));
    leveldb::Status status = db->Put(leveldb::WriteOptions(), key, slice);
    if(!status.ok()) throw std::runtime_error("Could not write transaction hash to tx db : " + status.ToString());
}

void TransactionStore::removeTransaction(Transaction& t) {
    SHA256Hash txHash = t.hashContents();
    leveldb::Slice key = leveldb::Slice((const char*) txHash.data(), txHash.size());
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
    if(!status.ok()) throw std::runtime_error("Could not remove transaction hash from tx db : " + status.ToString());
}
