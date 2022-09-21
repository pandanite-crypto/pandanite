#include <sstream>
#include "state_store.hpp"
using namespace std;

// NOTE: this is a first pass implementation, a lot of the code can be optimized

inline leveldb::Slice variableToSlice(const string& var, uint64_t blockId) {
    stringstream ss;
    ss << var + "___" << blockId;
    string s = ss.str();
    char * test = (char*) malloc(s.size() + 1);
    strcpy(test, s.c_str());
    leveldb::Slice s2 = leveldb::Slice((const char*) test, s.length() + 1);
    return s2;
}

vector<uint8_t> bytesToVector(uint8_t* buf, uint64_t sz) {
    vector<uint8_t> vec;
    for(int i = 0; i < sz; i++) {
        vec.push_back(buf[i]);
    }
    return std::move(vec);
}

void byteArrayToVector(char* buf, size_t sz, vector<uint8_t> & vec) {
    for(int i = 0; i < sz; i++) vec.push_back(buf[i]);
}

vector<uint8_t> stateToBuffer(const State& state) {
    // allocate buffer for storing state:
    uint64_t sz = sizeof(uint64_t)*3 + state.bytes.size();
    vector<uint8_t> ret;
    for (int i = 0; i < sz; i++) {
        ret.push_back(0);
    }
    uint8_t* arr = ret.data();
    uint64_t bufSize = state.bytes.size();
    memcpy(arr, &state.itemSize, sizeof(uint64_t));
    memcpy(arr + 1 * sizeof(uint64_t), &state.blockId, sizeof(uint64_t));
    memcpy(arr + 2 * sizeof(uint64_t), &state.lastBlockId, sizeof(uint64_t));
    memcpy(arr + 3 * sizeof(uint64_t), state.bytes.data(), state.bytes.size());
    return ret;
}
void bufferToState(vector<uint8_t>& buf,  State& state) {
    memcpy(&state.itemSize, buf.data(), sizeof(uint64_t));
    memcpy(&state.blockId, buf.data() + 1 * sizeof(uint64_t), sizeof(uint64_t));
    memcpy(&state.lastBlockId, buf.data() + 2 * sizeof(uint64_t), sizeof(uint64_t));
    uint64_t start = 3 * sizeof(uint64_t);
    for(int i = start; i < buf.size(); i++) {
        state.bytes.push_back(buf[i]);
    }
}

StateStore::StateStore() {
    this->currBlockId = 0;
}

void StateStore::addBlock() {
    this->currBlockId++;
}

void StateStore::popBlock() {
    cout<<"HERE"<<endl;
    for(auto variable : this->currentVariables) {
        State s;
        cout<<"A: "<< variable << endl;
        this->getKey(variable, s);
        if (s.lastBlockId >= 0) {
            // point variable to previous blocks instance
            cout<<"B"<<endl;
            this->variablesBlockIds[variable] = s.lastBlockId;
        } else {
            // variable no longer exists
            cout<<"C"<<endl;
            this->variablesBlockIds.erase(this->variablesBlockIds.find(variable));
        }
        cout<<"FOO"<<endl;
        this->deleteKey(variable);
        cout<<"BAR"<<endl;
    }
    cout<<"D"<<endl;
    this->currBlockId--;
    cout<<"E"<<endl;
    this->loadCurrentVariables();
    cout<<"A"<<endl;
}

void StateStore::saveCurrentVariables() {
    uint64_t sz = 0;
    for(auto var : this->currentVariables) {
        sz += var.size() + 1;
    }
    char* buf = (char*) malloc(sz);
    uint64_t delta = 0;
    for(auto var : this->currentVariables) {
        strcpy(buf + delta, var.c_str());
        delta += var.length() + 1;
    }
    string varName = "__variables_";
    leveldb::Slice s = variableToSlice(varName, this->currBlockId);
    leveldb::Status status = db->Put(leveldb::WriteOptions(), s, leveldb::Slice(buf, (size_t)sz));
    free(buf);
    if (!status.ok()) throw std::runtime_error("Could not save variable list");
}

void StateStore::loadCurrentVariables() {
    string varName = "__variables_";
    leveldb::Slice s = variableToSlice(varName, this->currBlockId);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), s, &value);
    if (!status.ok()) throw std::runtime_error("Could not load variable list");
    uint64_t sz = value.length() + 1;
    uint64_t offset = 0;
    const char* buf = value.c_str();
    this->currentVariables.clear();
    while (offset > sz) {
        const char* strptr = buf + offset;
        this->currentVariables.insert(string(strptr, strlen(strptr)));
        offset += strlen(strptr) + 1;
    }
}

bool StateStore::hasKey(const string& key) const{
    leveldb::Slice s = variableToSlice(key, this->currBlockId);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), s, &value);
    return status.ok();
}

bool StateStore::hasKeyAtBlock(const string& key, const uint64_t blockId) const{
    leveldb::Slice s = variableToSlice(key, blockId);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), s, &value);
    return status.ok();
}

void StateStore::deleteKey(const string& key) {
    if (!this->hasKey(key)) throw std::runtime_error("Tried to delete non-existent key");
    State curr;
    this->getKey(key, curr);
    this->variablesBlockIds[key] = curr.lastBlockId;
    currentVariables.erase(currentVariables.find(key));
    this->saveCurrentVariables();
    leveldb::Slice s = variableToSlice(key, this->currBlockId);
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), s);
    if(!status.ok()) throw std::runtime_error("Could not remove var from state db : " + status.ToString());
}

void StateStore::getKeyAtBlock(const string& key, State& buf, uint64_t blockId) const{
    if (!this->hasKeyAtBlock(key, blockId)) throw std::runtime_error("Tried to get non-existent key");
    leveldb::Slice s = variableToSlice(key, blockId);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), s, &value);
    vector<uint8_t> data;
    byteArrayToVector((char*)value.c_str(), value.size(), data);
    bufferToState(data, buf);
}

void StateStore::getKey(const string& key, State& buf) const {
    if (!this->hasKey(key)) throw std::runtime_error("Tried to get non-existent key");
    leveldb::Slice s = variableToSlice(key, this->currBlockId);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), s, &value);
    vector<uint8_t> data;
    byteArrayToVector((char*)value.c_str(), value.size(), data);
    bufferToState(data, buf);
}

void StateStore::putKey(const string& key, const State& state) {
    leveldb::Slice s = variableToSlice(key, this->currBlockId);
    if (currentVariables.find(key) == currentVariables.end()) {
        currentVariables.insert(key);
    }
    this->variablesBlockIds[key] = this->currBlockId;
    vector<uint8_t> buf = stateToBuffer(state);
    leveldb::Status status = db->Put(leveldb::WriteOptions(), s, leveldb::Slice((char*)buf.data(), buf.size()));
    if(!status.ok()) throw std::runtime_error("Could not write state to db : " + status.ToString());
}

uint64_t StateStore::count(const string& key) {
    State buf;
    this->getKey(key, buf);
    return buf.bytes.size() / buf.itemSize;
}

void StateStore::setBytes(const string& key, const vector<uint8_t>& bytes, const uint64_t idx) {
    if (this->hasKey(key)) {
        State curr;
        this->getKey(key, curr);
        if (curr.itemSize != bytes.size()) throw std::runtime_error("Item size does not match others in array");
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
        // if we have already seen this variable update the variablesBlockIds
        if (this->variablesBlockIds.find(key) != this->variablesBlockIds.end()) {
            curr.lastBlockId = this->variablesBlockIds[key];
        }
        this->variablesBlockIds[key] = this->currBlockId;
        curr.itemSize = bytes.size();
        curr.bytes = bytes;
        this->putKey(key, curr);
    }
}

void StateStore::setUint32(const string& key, const uint32_t& val, const uint64_t idx) {
    vector<uint8_t> bytes = bytesToVector((uint8_t*)&val, sizeof(uint32_t));
    this->setBytes(key, bytes, idx);
}

void StateStore::setUint64(const string& key, const uint64_t& val, const uint64_t idx) {
    vector<uint8_t> bytes = bytesToVector((uint8_t*)&val, sizeof(uint64_t));
    this->setBytes(key, bytes, idx);
}

void StateStore::setSha256(const string& key, const SHA256Hash& val, const uint64_t idx) {
    vector<uint8_t> bytes = bytesToVector((uint8_t*)val.data(), val.size());
    this->setBytes(key, bytes, idx);
}

void StateStore::setWallet(const string& key, const PublicWalletAddress& val, const uint64_t idx) {
    vector<uint8_t> bytes = bytesToVector((uint8_t*)val.data(), val.size());
    this->setBytes(key, bytes, idx);
}

void StateStore::setBigint(const string& key, const Bigint& val, const uint64_t idx) {
    string str = to_string(val);
    vector<uint8_t> bytes = bytesToVector((uint8_t*)str.c_str(), str.length());
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

vector<uint8_t> StateStore::getBytes(const string& key, uint64_t index) const {
    if (this->variablesBlockIds.find(key) == this->variablesBlockIds.end()) throw std::runtime_error("Tried to read non existent key");
    State curr;
    uint64_t block = this->variablesBlockIds.find(key)->second;
    this->getKeyAtBlock(key, curr, block);
    vector<uint8_t> ret;
    uint64_t start = index * curr.itemSize;
    for(int i = 0; i < curr.itemSize; i++) {
        ret.push_back(curr.bytes[start + i]);
    }
    return std::move(ret);
}

uint64_t StateStore::getUint64(const string& key, uint64_t index) const {
    return *(uint64_t*)(this->getBytes(key, index).data());
}

uint32_t StateStore::getUint32(const string& key, uint64_t index) const {
    vector<uint8_t> data = this->getBytes(key, index);
    return *(uint32_t*)data.data();
}

SHA256Hash StateStore::getSha256(const string& key, uint64_t index) const {
    SHA256Hash hash;
    vector<uint8_t> data = this->getBytes(key, index);
    memcpy(hash.data(), data.data(), data.size());
    return hash;
}

Bigint StateStore::getBigint(const string& key, uint64_t index) const {
    vector<uint8_t> data = this->getBytes(key, index);
    string value((char*)data.data(), data.size());
    return Bigint(value);
}

PublicWalletAddress StateStore::getWallet(const string& key, uint64_t index) const {
    PublicWalletAddress wallet;
    vector<uint8_t> data = this->getBytes(key, index);
    memcpy(wallet.data(), data.data(), data.size());
    return wallet;
}

void StateStore::remove(const string& key, const uint64_t idx) {
    if (!this->hasKey(key)) throw std::runtime_error("Tried to remove from non existent key");
    State curr;
    this->getKey(key, curr);

    if (curr.bytes.size() == 0) throw std::runtime_error("Tried to remove from empty vector");
    uint64_t numItems = curr.bytes.size() / curr.itemSize;
    if (idx >= numItems) throw std::runtime_error("Tried to remove from non-existent index");

    curr.bytes.erase(curr.bytes.begin() + (idx * numItems), curr.bytes.begin() + ((idx + 1) * numItems));

    this->putKey(key, curr);
}
