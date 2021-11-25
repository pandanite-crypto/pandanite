#pragma once
#include "common.hpp"
using namespace std;

class HostManager {
    public:
        HostManager(json config, string myName="");
        HostManager(); // only used for  mocks
        std::pair<string,int> getLongestChainHost();
        size_t size();
        void refreshHostList();
        vector<string> getHosts();
    protected:
        string myName;
        vector<string> hostSources;
        vector<string> hosts;
};

