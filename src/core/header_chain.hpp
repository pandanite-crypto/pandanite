#pragma once
#include <string>
#include <vector>
#include <thread>
#include "common.hpp"
#include "block.hpp"
using namespace std;

class HeaderChain {
    public:
        HeaderChain(string host);
        uint64_t getTotalWork();
        bool hostIsAlive();
        string getHost();
    protected:
        bool isAlive;
        string host;
        vector<BlockHeader> chain;
        uint64_t totalWork;
        vector<std::thread> syncThread;
        friend sync_blocks(HeaderChain & chain);
};