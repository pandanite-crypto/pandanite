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
    this->saveCurrentVariables();
    this->currBlockId++;
    cout<<"curr block id: " << this->currBlockId << endl;
    this->saveCurrentVariables();
}

void StateStore::setCurrentBlock(uint64_t blockId) {
    this->currBlockId = blockId;
    this->loadCurrentVariables();
    
}

void StateStore::printState() {
    SHA256Hash start = NULL_SHA256_HASH;
    SHA256Hash end;
    for(int i = 0; i < end.size(); i++) {
        end[i] = 255;
    }
    auto startSlice = leveldb::Slice((const char*) start.data(), start.size());
    auto endSlice = leveldb::Slice((const char*) end.data(), end.size());

    std::shared_ptr<leveldb::Iterator> it(db->NewIterator(leveldb::ReadOptions()));
    for(it->Seek(startSlice); it->Valid() && it->key().compare(endSlice) < 0; it->Next()) {                
        leveldb::Slice keySlice(it->key());
        cout<<"SLICE : " << string((char*)keySlice.data(), keySlice.size()) << endl;
    }
}
void StateStore::popBlock() {
    auto currentVars = this->currentVariables;
    for(auto variable : currentVars) {
        if (!(this->variablesBlockIds[variable] == this->currBlockId)) continue;
        State s;
        this->getKey(variable, s);
        if (s.lastBlockId >= 0) {
            // point variable to previous blocks instance
            this->variablesBlockIds[variable] = s.lastBlockId;
        } else {
            // variable no longer exists
            this->variablesBlockIds.erase(this->variablesBlockIds.find(variable));
        }
        this->deleteKey(variable);
    }
    this->currBlockId--;
    if (currBlockId > 0) {
        this->loadCurrentVariables();
    } else {
        this->currentVariables.clear();
        this->variablesBlockIds.clear();
    }
}

void StateStore::saveCurrentVariables() {
    if (this->currentVariables.size() != 0) {
        json data = variablesBlockIds;
        string varName = "__variables_";
        leveldb::Slice s = variableToSlice(varName, this->currBlockId);
        leveldb::WriteOptions write_options;
        write_options.sync = true;
        string str = data.dump();
        leveldb::Status status = db->Put(write_options, s, leveldb::Slice(str.c_str(), str.size()));
        if (!status.ok()) throw std::runtime_error("Could not save variable list");
    }
}

void StateStore::loadCurrentVariables() {
    string varName = "__variables_";
    leveldb::Slice s = variableToSlice(varName, this->currBlockId);
    string value;
    this->printState();
    leveldb::Status status = db->Get(leveldb::ReadOptions(), s, &value);
    if (!status.ok()) throw std::runtime_error("couldn't find variables");
    json data = json::parse(value);
    this->currentVariables.clear();
    for(auto& item : data.items()) {
        auto key = item.key();
        this->currentVariables.insert(key);
        this->variablesBlockIds[key] = item.value().get<uint64_t>();
        cout<<key<<","<<item.value()<<endl;
    }
}

set<string> StateStore::getCurrentVariables() const {
    return this->currentVariables;
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
    if (currentVariables.find(key) == currentVariables.end()) {
        throw std::runtime_error("Inconsistent current variables");
    }
    currentVariables.erase(currentVariables.find(key));
    this->saveCurrentVariables();
    leveldb::Slice s = variableToSlice(key, this->currBlockId);
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), s);
    if(!status.ok()) throw std::runtime_error("Could not remove var from state db : " + status.ToString());
}

void StateStore::getKeyAtBlock(const string& key, State& buf, uint64_t blockId) const{
    cout<<"GETTING KEY AT BLCOK " << key << blockId <<endl;
    if (!this->hasKeyAtBlock(key, blockId)) throw std::runtime_error("Tried to get non-existent key");
    leveldb::Slice s = variableToSlice(key, blockId);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), s, &value);
    vector<uint8_t> data;
    byteArrayToVector((char*)value.c_str(), value.size(), data);
    bufferToState(data, buf);
}

void StateStore::getKey(const string& key, State& buf) const {

    cout<<"GETTING KEY " << key <<endl;
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
    this->saveCurrentVariables();
    vector<uint8_t> buf = stateToBuffer(state);
    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::Status status = db->Put(write_options, s, leveldb::Slice((char*)buf.data(), buf.size()));
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
            for (auto item : bytes) {
                curr.bytes.push_back(item);
            }
        } else {
            // overwrite vector item
            uint64_t startBuf = curr.itemSize * idx;
            uint64_t i = 0;
            for (auto item : bytes) {
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

    curr.bytes.erase(curr.bytes.begin() + (idx * curr.itemSize), curr.bytes.begin() + ((idx + 1) * curr.itemSize));
    if (curr.bytes.size() == 0) {
        this->deleteKey(key);
    } else {
        this->putKey(key, curr);
    }
}
