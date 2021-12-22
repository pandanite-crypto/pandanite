#pragma once
#include <list>
#include <thread>
#include <vector>
#include <atomic>
#include "block.hpp"
using namespace std;

class HostManager;

class HeaderChain {
    public:
        HeaderChain(string host, HostManager* hm);
        uint64_t getTotalWork();
        uint64_t getChainLength();
        string getHostAddress();
        void setHost(string host);
        bool isReady();
    protected:
        HostManager* hostManager;
        void resetChain();
        void discardBlock();
        void popBlock();
        bool ready;
        atomic<uint64_t> totalWork;
        atomic<uint32_t> numBlocks;
        SHA256Hash lastHash;
        string host;
        vector<std::thread> syncThread;
        list<BlockHeader> headers;
        friend void sync_headers(HeaderChain& chain);
};