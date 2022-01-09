#pragma once
#include "constants.hpp"
#include "common.hpp"

class HeaderChain {
    public:
        HeaderChain(string host);
        void load();
        bool valid();
        string getHost();
        Bigint getTotalWork();
        uint64_t getChainLength();
        vector<SHA256Hash> blockHashes;
    protected:
        string host;
        Bigint totalWork;
        uint64_t chainLength;
        bool failed;
};