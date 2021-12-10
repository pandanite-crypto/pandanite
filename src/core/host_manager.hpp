#pragma once
#include "block.hpp"
#include "common.hpp"
#include <set>
#include <mutex>
using namespace std;

class HostManager {
    public:
        HostManager(json config, string myName="");
        HostManager(); // only used for  mocks
        string getBestHost();
        uint32_t getBestChainLength();
        uint64_t getBestChainWork();
        size_t size();
        void refreshHostList();
        vector<string> getHosts();
        void banHost(string host);
        bool isBanned(string host);
        void addPeer(string host);
        void propagateBlock(Block& b);
    protected:
        vector<string> sampleHosts(uint32_t num);
        std::mutex lock;
        vector<string> hostSources;
        set<string> bannedHosts;
        set<string> hosts;
        set<string> peers;
        void findBestHost();
        uint64_t downloadAndVerifyChain(string host);        
        string bestHost;
        uint32_t bestChainLength;
        uint64_t bestChainWork;
        string myName;
        
        
};

