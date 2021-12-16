#include "tx_store.hpp"
#include <thread>
#include <experimental/filesystem>


TransactionStore::TransactionStore() {
}

bool TransactionStore::hasTransaction(Transaction &t) {
    SHA256Hash txHash = t.getHash();
    leveldb::Slice key = leveldb::Slice((const char*) txHash.data(), txHash.size());
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    return (status.ok());

}

void TransactionStore::insertTransaction(Transaction& t) {
    SHA256Hash txHash = t.getHash();
    leveldb::Slice key = leveldb::Slice((const char*) txHash.data(), txHash.size());
    uint8_t dummy = 0;
    leveldb::Slice slice = leveldb::Slice((const char*)&dummy, sizeof(uint8_t));
    leveldb::Status status = db->Put(leveldb::WriteOptions(), key, slice);
    if(!status.ok()) throw std::runtime_error("Could not write transaction hash to tx db : " + status.ToString());
}

void TransactionStore::removeTransaction(Transaction& t) {
    SHA256Hash txHash = t.getHash();
    leveldb::Slice key = leveldb::Slice((const char*) txHash.data(), txHash.size());
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
    if(!status.ok()) throw std::runtime_error("Could not remove transaction hash from tx db : " + status.ToString());
}
