#pragma once
#include "constants.hpp"
#include "common.hpp"

class HeaderChain {
    public:
        HeaderChain(string host);
        void load();
        bool valid();
        string getHost();
        uint64_t getTotalWork();
        uint64_t getChainLength();
    protected:
        string host;
        uint64_t totalWork;
        uint64_t chainLength;
        bool failed;
};