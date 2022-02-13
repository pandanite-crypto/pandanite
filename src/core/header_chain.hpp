#pragma once
#include "constants.hpp"
#include "common.hpp"
#include <thread>
#include <map>
using namespace std;

class HeaderChain {
    public:
        HeaderChain(string host);
        void load();
        void reset();
        bool valid();
        string getHost();
        Bigint getTotalWork();
        uint64_t getChainLength();
        SHA256Hash getHash(uint64_t blockId);
        vector<SHA256Hash> blockHashes;
    protected:
        string host;
        Bigint totalWork;
        uint64_t chainLength;
        uint64_t offset;
        bool failed;
        map<uint64_t, SHA256Hash> checkPoints;
        vector<std::thread> syncThread;
        friend void chain_sync(HeaderChain& chain);
};