#include "pufferfish_cache.hpp"

#include <iostream>
#include <thread>
#include "../core/crypto.hpp"
#include "ledger.hpp"
using namespace std;


leveldb::Slice sha256ToSlice(const SHA256Hash& h) {
    leveldb::Slice s2 = leveldb::Slice((const char*) h.data(), h.size());
    return s2;
}


bool PufferfishCache::hasHash(const SHA256Hash& hash) const{
    std::string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), sha256ToSlice(hash), &value);
    return (status.ok());
}

SHA256Hash PufferfishCache::getHash(const SHA256Hash& hash) const{
    std::string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), sha256ToSlice(hash), &value);
    if (!status.ok()) throw std::runtime_error("Hash does not exist");
    return *((SHA256Hash*)value.c_str());
}
void PufferfishCache::setHash(const SHA256Hash& input, const SHA256Hash& value) {
    leveldb::Status status = db->Put(leveldb::WriteOptions(), sha256ToSlice(input), sha256ToSlice(value));
    if (!status.ok()) throw std::runtime_error("Write failed: " + status.ToString());
}
