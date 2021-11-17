#pragma once
#include "common.hpp"

class HostManager {
    public:
        HostManager(json config);
        HostManager(); // only used as mock
        std::pair<string,int> getLongestChainHost();
        size_t size();
    protected:
        vector<string> hosts;
        void refreshHostList();
};

