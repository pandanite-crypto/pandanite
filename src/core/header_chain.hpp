#pragma once
#include "constants.hpp"
#include "common.hpp"
#include "../server/block_store.hpp"
#include <thread>
#include <memory>
#include <map>
using namespace std;

class HeaderChain {
    public:
        HeaderChain(string host, map<uint64_t, SHA256Hash>& checkpoints, map<uint64_t, SHA256Hash>& bannedHashes, std::shared_ptr<BlockStore> blockStore = nullptr);
        void load();
        void reset();
        bool valid();
        string getHost() const;
        Bigint getTotalWork() const;
        uint64_t getChainLength() const;
        uint64_t getCurrentDownloaded() const;
        SHA256Hash getHash(uint64_t blockId) const;
        vector<SHA256Hash> blockHashes;
    protected:
        string host;
        Bigint totalWork;
        uint64_t chainLength;
        uint64_t offset;
        std::shared_ptr<BlockStore> blockStore;
        bool failed;
        bool triedBlockStoreCache;
        map<uint64_t, SHA256Hash> checkPoints;
        map<uint64_t, SHA256Hash> bannedHashes;
        vector<std::thread> syncThread;
        friend void chain_sync(HeaderChain& chain);
};
