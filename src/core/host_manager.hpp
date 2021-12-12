#pragma once
#include "common.hpp"
using namespace std;

class HostManager {
    public:
        HostManager(json config, string myName="");
        HostManager(); // only used for  mocks
        std::pair<string,uint64_t> getBestHost();
        size_t size();
        void refreshHostList();
        vector<string> getHosts();
        void addPeer(string addr);
    protected:
        string myName;
        vector<string> hostSources;
        vector<string> hosts;
};

