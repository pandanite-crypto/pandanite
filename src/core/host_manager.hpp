#pragma once
#include "common.hpp"
#include <set>
using namespace std;

class HeaderChain;

class HostManager {
    public:
        HostManager(json config, string myName="");
        HostManager(); // only used for  mocks
        std::pair<string,uint64_t> getBestHost();
        size_t size();
        void refreshHostList(bool resampleHeadChains=false);
        vector<string> getHosts(bool includeSelf=true);
        set<string> sampleHosts(int count);
        string getUnusedHost();
        void addPeer(string addr);
        bool isDisabled();
        bool isReady();
    protected:
        bool disabled;
        string myAddress;
        string myName;
        vector<string> hostSources;
        vector<string> hosts;
        vector<HeaderChain*> currentHosts;
};

