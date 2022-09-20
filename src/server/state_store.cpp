#include "state_store.hpp"


inline leveldb::Slice variableToSlice(const string& var, uint64_t blockId) {
    char buf[1024];
    strcpy(buf, var.c_str());
    memcpy(buf + strlen(var.c_str()) + 1, &blockId, sizeof(uint64_t));
    uint64_t bufSz = strlen(var.c_str()) + 1 + sizeof(uint64_t);
    leveldb::Slice s2 = leveldb::Slice((const char*) buf, bufSz);
    return s2;
}

vector<uint8_t> bytesToVector(char* buf, uint64_t sz) {
    vector<uint8_t> vec;
    for(int i = 0; i < sz; i++) {
        vec.push_back(buf[i]);
    }
    return std::move(vec);
}

void* stateToBuffer(State& state) {
    // allocate buffer for storing state:
    uint64_t sz = sizeof(uint64_t)*4 + bytes.size();
    void* ret = malloc(sz);
    
    uint64_t bufSize = state.bytes.size();
    memcpy(ret, &state.itemSize, sizeof(uint64_t));
    memcpy(ret + 1 * sizeof(uint64_t), &state.blockId, sizeof(uint64_t));
    memcpy(ret + 2 * sizeof(uint64_t), &state.lastBlockId, sizeof(uint64_t));
    memcpy(ret + 3 * sizeof(uint64_t), &bufSize, sizeof(uint64_t));
    memcpy(ret + 4 * sizeof(uint64_t), bytes.data(), bytes.size());
    return ret;
}
void bufferToState(void* buf, State& state) {
    memcpy(&state.itemSize, buf, sizeof(uint64_t));
    memcpy(&state.blockId, buf + 1 * sizeof(uint64_t), sizeof(uint64_t));
    memcpy(&state.lastBlockId, buf + 2 * sizeof(uint64_t), sizeof(uint64_t));
    uint64_t bufSize;
    memcpy(&bufSize, buf + 3 * sizeof(uint64_t), sizeof(uint64_t));
    uint8_t* buf = (uint8_t*) (buf + 4 * sizeof(uint64_t));
    for(int i = 0;i < bufSize; i++) {
        state.bytes.push_back(buf[i]);
    }
}

StateStore::StateStore() {
    this->currBlockId = 0;
}

void StateStore::addBlock() {
    this->currBlockId++;
}

bool StateStore::hasKey(const string& key) {
    leveldb::Slice s = variableToSlice(key, this->currBlockId);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), s, &value);
    return status.ok();
}

void StateStore::deleteKey(const string& key) {
    if (!this->hasKey(key)) throw std::runtime_error("Tried to delete non-existent key");
    leveldb::Slice s = variableToSlice(key, this->currBlockId);
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), s);
    if(!status.ok()) throw std::runtime_error("Could not remove var from state db : " + status.ToString());
}

void StateStore::getKey(const string& key, State& buf) {
    if (!this->hasKey(key)) throw std::runtime_error("Tried to get non-existent key");
    leveldb::Slice s = variableToSlice(key, this->currBlockId);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), s, &value);
    bufferToState(&value, buf);
}

void StateStore::putKey(const string& key, const State& state) {
    leveldb::Slice s = variableToSlice(key, this->currBlockId);
    void* buf = stateToBuffer(state);
    leveldb::Status status = db->Put(leveldb::WriteOptions(), s, buf);
    if(!status.ok()) {
        free(buf);
        throw std::runtime_error("Could not write state to db : " + status.ToString());
    }
    free(buf);
}

uint64_t StateStore::count(const string& key) {
    State buf;
    this->getKey(key, buf);
    return buf.bytes.size() / buf.itemSize;
}

void StateStore::setBytes(const string& key, const vector<uint8_t>& bytes, const uint64_t idx = 0) {
    if (this->hasKey(key)) {
        State curr;
        this->getKey(key, curr);
        if (curr.itemSize ! = bytes.size()) throw std::runtime_error("Item size does not match others in array");
        // check if the item is within 1 of array size
        uint64_t sz = curr.bytes.size() / curr.itemSize;
        if (idx < 0 || idx > sz) throw std::runtime_error("Item index is greater than length of current array");
        if (sz == idx) {
            // append to vector
            for (auto item : curr.bytes) {
                curr.bytes.push_back(item);
            }
        } else {
            // overwrite vector item
            uint64_t startBuf = curr.itemSize * idx;
            uint64_t i = 0;
            for (auto item : curr.bytes) {
                curr.bytes[startBuf + i] = item;
                i++;
            }
        }
        this->putKey(key, curr);
    } else {
        if (idx != 0) throw std::runtime_error("Tried to write at index greater than 0 of non existent variable");
        // add this variable to the list of current variables in the state and store in leveldb
        State curr;
        curr.blockId = this->currBlockId;
        curr.lastBlockId = -1;
        curr.itemSize = bytes.size();
        curr.bytes = bytes;
        this->putKey(key, curr);
    }
}


void StateStore::setUint32(const string& key, const uint32_t& val, const uint64_t idx = 0) {
    vector<uint8_t> bytes = bytesToVector(&val, sizeof(uint32_t));
    this->setBytes(key, bytes, idx);
}

void StateStore::setUint64(const string& key, const uint64_t& val, const uint64_t idx = 0) {
    vector<uint8_t> bytes = bytesToVector(&val, sizeof(uint64_t));
    this->setBytes(key, bytes, idx);
}

void StateStore::setSha256(const string& key, const SHA256Hash& val, const uint64_t idx = 0) {
    vector<uint8_t> bytes = bytesToVector(val.data(), val.size());
    this->setBytes(key, bytes, idx);
}

void StateStore::setWallet(const string& key, const PublicWalletAddress& val, const uint64_t idx = 0) {
    vector<uint8_t> bytes = bytesToVector(val.data(), val.size());
    this->setBytes(key, bytes, idx);
}

void StateStore::setBigint(const string& key, const Bigint& val, const uint64_t idx = 0) {
    string str = std::to_string(val);
    vector<uint8_t> bytes = bytesToVector(str.c_str(), str.length());
    this->setBytes(key, bytes, idx);
}

void StateStore::pop(const string& key) {
    if (!this->hasKey(key)) throw std::runtime_error("Tried to pop non-existent key");
    State curr;
    this->getKey(key, curr);
    for(int i = 0; i < curr.itemSize; i++) {
        curr.bytes.pop_back();
    }
    this->putKey(key, curr);
}

uint64_t StateStore::getUint64(uint64_t index = 0) const {

}

uint32_t StateStore::getUint32(uint64_t index = 0) const {

}

SHA256Hash StateStore::getSha256(uint64_t index = 0) const {

}

Bigint StateStore::getBigint(uint64_t index = 0) const {

}

PublicWalletAddress StateStore::getWallet(uint64_t index = 0) const {

}

vector<uint8_t> StateStore::getBytes(uint64_t index = 0) const {

}

void removeBytes(const string& key, const uint64_t idx) {

}
void removeUint32(const string& key, const uint64_t idx);
void removeUint64(const string& key, const uint64_t idx);
void removeSha256(const string& key, const uint64_t idx);
void removeWallet(const string& key, const uint64_t idx);
void removeBigint(const string&key, const uint64_t idx);