#pragma once
#include "common.hpp"
#include "header_chain.hpp"
#include "../server/block_store.hpp"
#include <set>
#include <mutex>
#include <memory>
#include <thread>
using namespace std;

class HostManager {
    public:
        HostManager(json config);
        HostManager(); // only used for  mocks
        ~HostManager();
        size_t size();
        string computeAddress();
        void refreshHostList();
        void startPingingPeers();
        string getGoodHost() const;
        uint64_t getBlockCount() const;
        Bigint getTotalWork() const;
        SHA256Hash getBlockHash(string host, uint64_t blockId) const;
        map<string,uint64_t> getHeaderChainStats() const;
        std::pair<string,uint64_t> getRandomHost() const;
        vector<string> getHosts(bool includeSelf=true) const;
        set<string> sampleFreshHosts(int count);
        set<string> sampleAllHosts(int count);
        string getAddress()const;
        uint64_t getNetworkTimestamp()const;
        void setBlockstore(std::shared_ptr<BlockStore> blockStore);
        void addPeer(string addr, uint64_t time, string version, string network);
        bool isDisabled();
        void syncHeadersWithPeers();
    protected:
        vector<std::shared_ptr<HeaderChain>> currPeers; 
        std::shared_ptr<BlockStore> blockStore;
        mutable std::mutex lock;
        bool disabled;
        bool firewall;
        string ip;
        int port;
        string name;
        string address;
        string version;
        string minHostVersion;
        string networkName;
        map<string,uint64_t> hostPingTimes;
        map<string,int32_t> peerClockDeltas;
        map<uint64_t, SHA256Hash> checkpoints;
        map<uint64_t, SHA256Hash> bannedHashes;
        vector<string> hostSources;
        vector<string> hosts;
        set<string> blacklist;
        set<string> whitelist;
        vector<std::thread> syncThread;
        vector<std::thread> headerStatsThread;
        friend void peer_sync(HostManager& hm);
};

