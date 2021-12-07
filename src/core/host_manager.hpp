#pragma once
#include "common.hpp"
#include <set>
using namespace std;

class HostManager {
    public:
        HostManager(json config, string myName="");
        HostManager(); // only used for  mocks
        string getBestHost();
        size_t size();
        void refreshHostList();
        vector<string> getHosts();
        void banHost(string host);
        bool isBanned(string host);
    protected:
        set<string> bannedHosts;
        void initBestHost();
        uint64_t downloadAndVerifyChain(string host);
        string bestHost;
        string myName;
        vector<string> hostSources;
        vector<string> hosts;
        vector<string> verifiedHosts;
};

