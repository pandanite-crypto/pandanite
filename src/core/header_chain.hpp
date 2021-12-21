#pragma once
#include <list>
#include <thread>
#include <vector>
#include <atomic>
#include "block.hpp"
using namespace std;

class HeaderChain {
    public:
        HeaderChain(string host);
        uint64_t getTotalWork();
        uint32_t getChainLength();
        void setStartBlock(BlockHeader& b, uint64_t totalWork);
    protected:
        void resetChain();
        void discardBlock();
        void popBlock();
        atomic<uint64_t> totalWork;
        atomic<uint32_t> numBlocks;
        SHA256Hash lastHash;
        string host;
        vector<std::thread> syncThread;
        list<BlockHeader> headers;
        friend sync_headers(HeaderChain& chain);
};