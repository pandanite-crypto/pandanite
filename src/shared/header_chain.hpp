#pragma once
#include "../core/constants.hpp"
#include "../core/common.hpp"
#ifndef WASM_BUILD
#include "../server/block_store.hpp"
#endif
#include <thread>
#include <memory>
#include <map>
using namespace std;

class HeaderChain {
    public:
<<<<<<< HEAD
        HeaderChain(string host, map<uint64_t, SHA256Hash>& checkpoints, map<uint64_t, SHA256Hash>& bannedHashes, std::shared_ptr<BlockStore> blockStore = nullptr);
=======
#ifndef WASM_BUILD
        HeaderChain(string host, map<uint64_t, SHA256Hash>& checkpoints, map<uint64_t, SHA256Hash>& bannedHashes, BlockStore* blockStore = NULL);
#else
        HeaderChain(string host, map<uint64_t, SHA256Hash>& checkpoints, map<uint64_t, SHA256Hash>& bannedHashes);
#endif
>>>>>>> 7ea4221... checkpoint
        void load();
        void reset();
        bool valid();
        string getHost();
        Bigint getTotalWork();
        uint64_t getChainLength();
        uint64_t getCurrentDownloaded();
        SHA256Hash getHash(uint64_t blockId);
        vector<SHA256Hash> blockHashes;
    protected:
        string host;
        Bigint totalWork;
        uint64_t chainLength;
        uint64_t offset;
<<<<<<< HEAD
        std::shared_ptr<BlockStore> blockStore;
=======
>>>>>>> 7ea4221... checkpoint
        bool failed;
        bool triedBlockStoreCache;
        map<uint64_t, SHA256Hash> checkPoints;
        map<uint64_t, SHA256Hash> bannedHashes;
        vector<std::thread> syncThread;
        friend void chain_sync(HeaderChain& chain);
#ifndef WASM_BUILD
        BlockStore* blockStore;
#endif
};