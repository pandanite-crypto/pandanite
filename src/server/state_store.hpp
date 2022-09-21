#pragma once
#include <string>
#include <vector>
#include <set>
#include "leveldb/db.h"
#include "data_store.hpp"
#include "../core/common.hpp"
using namespace std;


struct State {
    uint64_t blockId;
    uint64_t lastBlockId;
    uint64_t itemSize;
    vector<uint8_t> bytes;
};

vector<uint8_t> stateToBuffer(const State& state);
void bufferToState(vector<uint8_t>& buf, State& state);

class StateStore : public DataStore {
    public:
        StateStore();
        void addBlock();
        void popBlock();
        bool hasKey(const string& key) const;
        bool hasKeyAtBlock(const string& key, const uint64_t blockId) const;
        void deleteKey(const string& key);
        void pop(const string& key);
        void remove(const string& key, const uint64_t idx);
        uint64_t count(const string& key);
        void setUint32(const string& key, const uint32_t& val, const uint64_t idx = 0);
        void setUint64(const string& key, const uint64_t& val, const uint64_t idx = 0);
        void setSha256(const string& key, const SHA256Hash& val, const uint64_t idx = 0);
        void setWallet(const string& key, const PublicWalletAddress& val, const uint64_t idx = 0);
        void setBytes(const string& key, const vector<uint8_t>& bytes, const uint64_t idx = 0);
        void setBigint(const string& key, const Bigint& val, const uint64_t idx = 0);
        uint64_t getUint64(const string& key, uint64_t index = 0) const;
        uint32_t getUint32(const string& key, uint64_t index = 0) const;
        SHA256Hash getSha256(const string& key, uint64_t index = 0) const;
        Bigint getBigint(const string& key, uint64_t index = 0) const;
        PublicWalletAddress getWallet(const string& key, uint64_t index = 0) const;
        vector<uint8_t> getBytes(const string& key, uint64_t index = 0) const;
        void getKeyAtBlock(const string& key, State& buf, uint64_t blockId) const;
    protected:
        map<string,uint64_t> variablesBlockIds;
        void putKey(const string& key, const State& buf);
        void getKey(const string& key, State& buf) const;
        void saveCurrentVariables();
        void loadCurrentVariables();
        set<string> currentVariables;
        uint64_t currBlockId;
};