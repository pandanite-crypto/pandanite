#pragma once
#include "common.hpp"
#include <set>
#include <mutex>
 #include <thread>
using namespace std;

class HostManager {
    public:
        HostManager(json config);
        HostManager(); // only used for  mocks
        size_t size();
        string computeAddress();
        void refreshHostList();
        void initTrustedHost();

        SHA256Hash getBlockHash(uint64_t blockId);
        std::pair<string,uint64_t> getTrustedHost();
        uint64_t getTrustedHostWork();
        std::pair<string,uint64_t> getRandomHost();
        vector<string> getHosts(bool includeSelf=true);
        set<string> sampleHosts(int count);
        string getAddress();
        
        void addPeer(string addr);
        bool isDisabled();
        
        void acquire();
        void release();
    protected:
        std::mutex lock;
        bool disabled;
        bool hasTrustedHost;
        string ip;
        int port;
        string name;
        string address;
        
        std::pair<string,uint64_t> trustedHost;
        uint64_t trustedWork;
        
        map<string,uint64_t> hostPings;
        vector<string> hostSources;
        vector<string> hosts;
        vector<SHA256Hash> validationHashes;

        vector<std::thread> syncThread;
        friend void peer_sync(HostManager& hm);
};

