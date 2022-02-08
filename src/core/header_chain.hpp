#pragma once
#include "constants.hpp"
#include "common.hpp"
#include <list>
#include <thread>
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
        list<SHA256Hash> blockHashes;
    protected:
        string host;
        Bigint totalWork;
        uint64_t chainLength;
        uint64_t offset;
        bool failed;
        vector<std::thread> syncThread;
        friend void chain_sync(HeaderChain& chain);
};