#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "request.hpp"
#include "logger.hpp"
#include "header_chain.hpp"
#include "../external/http.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <cstdio>
using namespace std;

#define ADD_PEER_BRANCH_FACTOR 10
#define RANDOM_GOOD_HOST_COUNT 1
#define HOST_MIN_FRESHNESS 180 * 60 // 3 hours

/*
    Fetches the public IP of the node
*/

string HostManager::computeAddress() {
    return this->address;
}

/*
    This thread periodically updates all neighboring hosts with the 
    nodes current IP 
*/  
void peer_sync(HostManager* hm) {
    while(true) {
        for(auto host : hm->hosts) {
            try {
                if (hm->address != "") {
                    pingPeer(host, hm->computeAddress(), std::time(0), hm->version, hm->networkName);
                }
            } catch (...) { }
        }
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}

HostManager::HostManager(json config) {
    this->name = config["name"];
    this->port = config["port"];
    this->ip = config["ip"];
    this->version = BUILD_VERSION;
    this->networkName = config["networkName"];
    this->address = "";

    // parse checkpoints
    for(auto checkpoint : config["checkpoints"]) {
        this->checkpoints.insert(std::pair<uint64_t, SHA256Hash>(checkpoint[0], stringToSHA256(checkpoint[1])));
    }

    // parse supported host versions
    this->minHostVersion = config["minHostVersion"];

    // check if a blacklist file exists
    std::ifstream blacklist("blacklist.txt");
    if (blacklist.good()) {
        std::string line;
        while (std::getline(blacklist, line)) {
            if (line[0] != '#') {
                string blocked = line;
                if (blocked[blocked.size() - 1] == '/') {
                    blocked = blocked.substr(0, blocked.size() - 1);
                }
                this->blacklist.insert(blocked);
                Logger::logStatus("Ignoring host " + blocked);
            }
        }
    }

    // check if a whitelist file exists
    std::ifstream whitelist("whitelist.txt");
    if (whitelist.good()) {
        std::string line;
        while (std::getline(whitelist, line)) {
            if (line[0] != '#') {
                string enabled = line;
                if (enabled[enabled.size() - 1] == '/') {
                    enabled = enabled.substr(0, enabled.size() - 1);
                }
                this->whitelist.insert(enabled);
                Logger::logStatus("Enabling host " + enabled);
            }
        }
    }

    this->disabled = false;
    for(auto h : config["hostSources"]) {
        this->hostSources.push_back(h);
    }
    this->refreshHostList();
}

void HostManager::startPingingPeers() {
    if (this->syncThread.size() > 0) throw std::runtime_error("Peer ping thread exists.");
    printf("Address of x is in calling thread is %p\n", (void *)this);  
    this->syncThread.push_back(std::thread(peer_sync, this));
}

void HostManager::setAddress(string addr) {
    Logger::logStatus("Set node address to: " + addr);
    this->address = addr;
    this->startPingingPeers();
}

string HostManager::getAddress() {
    return this->address;
}

// Only used for tests
HostManager::HostManager() {
    this->disabled = true;
}

uint64_t HostManager::getNetworkTimestamp() {
    // find deltas of all hosts that pinged recently
    vector<int32_t> deltas;
    for (auto pair : this->hostPingTimes) {
        uint64_t lastPingAge = std::time(0) - pair.second;
        // only use peers that have pinged recently
        if (lastPingAge < HOST_MIN_FRESHNESS) { 
            deltas.push_back(this->peerClockDeltas[pair.first]);
        }
    }
    
    if (deltas.size() == 0) return std::time(0);

    std::sort(deltas.begin(), deltas.end());
    
    // compute median
    uint64_t medianTime;
    if (deltas.size() % 2 == 0) {
        int32_t avg = (deltas[deltas.size()/2] + deltas[deltas.size()/2 - 1])/2;
        medianTime = std::time(0) + avg;
    } else {
        int32_t delta = deltas[deltas.size()/2];
        medianTime = std::time(0) + delta;
    }

    return medianTime;
}   

/*
    Asks nodes for their current POW and chooses the best peer
*/
string HostManager::getGoodHost() {
    if (this->currPeers.size() < 1) return "";
    Bigint bestWork = 0;
    string bestHost = this->currPeers[0]->getHost();
    this->lock.lock();
    for(auto h : this->currPeers) {
        if (h->getTotalWork() > bestWork) {
            bestWork = h->getTotalWork();
            bestHost = h->getHost();
        }
    }
    this->lock.unlock();
    return bestHost;
}

/*
    Returns the block count of the highest PoW chain amongst current peers
*/
uint64_t HostManager::getBlockCount() {
    if (this->currPeers.size() < 1) return 0;
    uint64_t bestLength = 0;
    Bigint bestWork = 0;
    this->lock.lock();
    for(auto h : this->currPeers) {
        if (h->getTotalWork() > bestWork) {
            bestWork = h->getTotalWork();
            bestLength = h->getChainLength();
        }
    }
    this->lock.unlock();
    return bestLength;
}

/*
    Returns the total work of the highest PoW chain amongst current peers
*/
Bigint HostManager::getTotalWork() {
    Bigint bestWork = 0;
    if (this->currPeers.size() < 1) return bestWork;
    this->lock.lock();
    for(auto h : this->currPeers) {
        if (h->getTotalWork() > bestWork) {
            bestWork = h->getTotalWork();
        }
    }
    this->lock.unlock();
    return bestWork;
}

/*
    Returns the block header hash for the given block, peer host
 */
SHA256Hash HostManager::getBlockHash(string host, uint64_t blockId) {
    SHA256Hash ret = NULL_SHA256_HASH;
    this->lock.lock();
    for(auto h : this->currPeers) {
        if (h->getHost() == host) {
            ret = h->getHash(blockId);
            break;
        }
    }
    this->lock.unlock();
    return ret;
}


/*
    Returns N unique random hosts that have pinged us
*/
set<string> HostManager::sampleFreshHosts(int count) {
    vector<string> freshHosts;
    for (auto pair : this->hostPingTimes) {
        uint64_t lastPingAge = std::time(0) - pair.second;
        // only return peers that have pinged
        if (lastPingAge < HOST_MIN_FRESHNESS) { 
            freshHosts.push_back(pair.first);
        }
    }

    int numToPick = min(count, (int)freshHosts.size());
    set<string> sampledHosts;
    while(sampledHosts.size() < numToPick) {
        string host = freshHosts[rand()%freshHosts.size()];
        sampledHosts.insert(host);
    }
    return sampledHosts;
}


/*
    Returns any N unique random hosts. 
    Used when hosts don't ping us (Miner)
*/
set<string> HostManager::sampleAllHosts(int count) {
    int numToPick = min(count, (int)this->hosts.size());
    set<string> sampledHosts;
    while(sampledHosts.size() < numToPick) {
        string host = this->hosts[rand()%this->hosts.size()];
        sampledHosts.insert(host);
    }
    return sampledHosts;
}


/*
    Adds a peer to the host list, 
*/
void HostManager::addPeer(string addr, uint64_t time, string version, string network) {
    if (network != this->networkName) return;
    if (version < this->minHostVersion) return;

    // check if host is in blacklist
    if (this->blacklist.find(addr) != this->blacklist.end()) return;

    // check if we already have this peer host
    auto existing = std::find(this->hosts.begin(), this->hosts.end(), addr);
    if (existing != this->hosts.end()) {
        this->hostPingTimes[addr] = std::time(0);
        // record how much our system clock differs from theirs:
        this->peerClockDeltas[addr] = std::time(0) - time;
        return;
    } 

    // check if the host is reachable:
    try {
        json name = getName(addr);
    } catch(...) {
        // if not exit
        return;
    }
    
    // add to our host list
    if (this->whitelist.size() == 0 || this->whitelist.find(addr) != this->whitelist.end()){
        Logger::logStatus("Added new peer: " + addr);
        hosts.push_back(addr);
    } else {
        return;
    }

    // check if we have less peers than needed, if so add this to our peer list
    if (this->currPeers.size() < RANDOM_GOOD_HOST_COUNT) {
        this->lock.lock();
        this->currPeers.push_back(new HeaderChain(addr, this->checkpoints));
        this->lock.unlock();
    }

    // pick random neighbor hosts and forward the addPeer request to them:
    set<string> neighbors = this->sampleFreshHosts(ADD_PEER_BRANCH_FACTOR);
    vector<future<void>> reqs;
    string _version = this->version;
    string networkName = this->networkName;
    for(auto neighbor : neighbors) {
        reqs.push_back(std::async([neighbor, addr, _version, networkName](){
            if (neighbor == addr) return;
            try {
                pingPeer(neighbor, addr, std::time(0), _version, networkName);
            } catch(...) {
                Logger::logStatus("Could not add peer " + addr + " to " + neighbor);
            }
        }));
    }

    for(int i =0 ; i < reqs.size(); i++) {
        reqs[i].get();
    }   
}


bool HostManager::isDisabled() {
    return this->disabled;
}


/*
    Downloads an initial list of peers and validates connectivity to them
*/
void HostManager::refreshHostList() {
    if (this->hostSources.size() == 0) return;

    Logger::logStatus("Finding peers...");

    set<string> fullHostList;

    // Iterate through all host sources merging into a combined peer list
    for (int i = 0; i < this->hostSources.size(); i++) {
        try {
            string hostUrl = this->hostSources[i];
            auto response = sendGetRequest(hostUrl, 10000);
            json hostList = json::parse(std::string{response.begin(), response.end()});
            for(auto host : hostList) {
                fullHostList.insert(string(host));
            }
        } catch (...) {
            continue;
        }
    }

    if (fullHostList.size() == 0) return;

    // iterate through all listed peer hosts
    vector<std::thread> threads;
    std::mutex lock;
    for(auto hostJson : fullHostList) {
        // if we've already added this host skip
        string hostUrl = string(hostJson);

        auto existing = std::find(this->hosts.begin(), this->hosts.end(), hostUrl);
        if (existing != this->hosts.end()) continue;

        // if host is in blacklist skip:
        if (this->blacklist.find(hostUrl) != this->blacklist.end()) continue;

        if (hostUrl.find("proxy") != std::string::npos) {
            cout<<"SKIPPING : "<<hostUrl    <<endl;
            continue;
        }
        // otherwise try connecting to the host to confirm it's up
        HostManager& hm = *this;
        threads.emplace_back(
            std::thread([hostUrl, &hm, &lock](){
                try {
                    json hostInfo = getName(hostUrl);
                    if (hostInfo["version"] < hm.minHostVersion) {
                        Logger::logStatus(RED + "[ UNREACHABLE ] " + RESET  + hostUrl);
                        return;
                    }
                    lock.lock();
                    if (hm.whitelist.size() == 0 || hm.whitelist.find(hostUrl) != hm.whitelist.end()){
                        hm.hosts.push_back(hostUrl);
                        Logger::logStatus(GREEN + "[ CONNECTED ] " + RESET  + hostUrl);
                        hm.hostPingTimes[hostUrl] = std::time(0);
                    }
                    lock.unlock();
                    
                } catch (...) {
                    Logger::logStatus(RED + "[ UNREACHABLE ] " + RESET  + hostUrl);
                }
            })
        );
    }
    for (auto& th : threads) th.join();
    Logger::logStatus("Peer list size = " + to_string(this->hosts.size()));
    this->syncHeadersWithPeers();
}

void HostManager::syncHeadersWithPeers() {
    // free existing peers
    this->lock.lock();
    for(auto peer : this->currPeers) {
        delete peer;
    }
    this->currPeers.empty();
    
    // pick N random peers
    set<string> hosts = this->sampleAllHosts(RANDOM_GOOD_HOST_COUNT);

    for (auto h : hosts) {
        this->currPeers.push_back(new HeaderChain(h, this->checkpoints));
    }
    this->lock.unlock();
}


/*
    Returns a list of all peer hosts
*/
vector<string> HostManager::getHosts(bool includeSelf) {
    vector<string> ret = this->hosts;
    if (includeSelf) ret.push_back(this->address);
    return ret;
}

size_t HostManager::size() {
    return this->hosts.size();
}