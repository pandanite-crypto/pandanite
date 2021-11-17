#pragma once
#include "common.hpp"

class HostManager {
    public:
        HostManager(json config);
        HostManager(); // only used as mock
        std::pair<string,int> getLongestChainHost();
        size_t size();
        void refreshHostList();
        vector<string> getHosts();
    protected:
        vector<string> hostSources;
        vector<string> hosts;
};

