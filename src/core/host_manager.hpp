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
        Bigint getTrustedHostWork();
        string getGoodHost();
        std::pair<string,uint64_t> getRandomHost();
        vector<string> getHosts(bool includeSelf=true);
        set<string> sampleHosts(int count);
        string getAddress();
        uint64_t getNetworkTimestamp();
        
        void addPeer(string addr, uint64_t time, string version);
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
        string version;
        
        std::pair<string,uint64_t> trustedHost;
        Bigint trustedWork;
        
        map<string,uint64_t> hostPingTimes;
        map<string,int32_t> peerClockDeltas;
        vector<string> hostSources;
        vector<string> hosts;
        vector<SHA256Hash> validationHashes;

        vector<std::thread> syncThread;
        friend void peer_sync(HostManager& hm);
};

