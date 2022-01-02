#pragma once
#include "common.hpp"
#include <set>
using namespace std;

class HostManager {
    public:
        HostManager(json config, string myName="");
        HostManager(); // only used for  mocks
        size_t size();
        void refreshHostList();
        void initTrustedHost();
        std::pair<string,uint64_t> getTrustedHost();
        vector<string> getHosts(bool includeSelf=true);
        set<string> sampleHosts(int count);
        
        void addPeer(string addr);
        bool isDisabled();
    protected:
        bool disabled;
        bool hasTrustedHost;
        string myAddress;
        string myName;
        
        std::pair<string,uint64_t> trustedHost;
        vector<string> hostSources;
        vector<string> hosts;
};

