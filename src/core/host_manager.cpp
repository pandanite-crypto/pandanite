#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "header_chain.hpp"
#include "../external/http.hpp"
#include <iostream>
#include <thread>
#include <future>
using namespace std;

#define ADD_PEER_BRANCH_FACTOR 10
#define HEADER_VALIDATION_HOST_COUNT 5




/*
    Fetches the public IP of the node
*/
string computeAddress() {
    string rawUrl = exec("curl -s ifconfig.co") ;
    return "http://" + rawUrl.substr(0, rawUrl.size() - 1) + ":3000";
}

/*
    This thread periodically updates all neighboring hosts with the 
    nodes current IP 
*/  
void peer_sync(HostManager& hm) {
    while(true) {
        hm.myAddress = computeAddress();
        for(auto host : hm.hosts) {
            try {
                addPeerNode(host, hm.myAddress);
            } catch (...) { }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300000));
    }
}

HostManager::HostManager(json config, string myName) {
    this->myName = myName;
    this->myAddress = computeAddress();
    this->disabled = false;
    this->hasTrustedHost = false;
    this->trustedWork = 0;
    for(auto h : config["hostSources"]) {
        this->hostSources.push_back(h);
    }
    if (this->hostSources.size() == 0) {
        this->hosts.push_back("http://localhost:3000");
    } else {
        this->refreshHostList();
        this->syncThread.push_back(std::thread(peer_sync, ref(*this)));
    }
}

// Only used for tests
HostManager::HostManager() {
    this->disabled = true;
}


/*
    Loads header chains from peers and validates them, storing hash headers from highest POW chain
*/
void HostManager::initTrustedHost() {
    // pick random hosts
    set<string> hosts = this->sampleHosts(HEADER_VALIDATION_HOST_COUNT);

    if (hosts.size() == 0) {
        Logger::logStatus("No hosts found");
    }

    vector<HeaderChain> chains;

    // fetch block headers from each host
    int i = 0;
    // TODO: this absolutely needs to be multi-threaded
    for(auto host : hosts) {
        chains.push_back(HeaderChain(host));
        chains[i].load();
        i++;
    }

    // pick the best (highest POW) header chain as host:
    uint64_t bestWork = 0;
    uint64_t bestLength = 0;
    string bestHost = "";
    
    for(auto chain : chains) {
        if (chain.valid()) {
            if (chain.getTotalWork() > bestWork) {
                bestWork = chain.getTotalWork();
                bestLength = chain.getChainLength();
                bestHost = chain.getHost();
                // store the validation hashes 
                this->validationHashes = chain.blockHashes;
            }
        }
    }

    if (bestHost == "") {
        throw std::runtime_error("Could not find valid header chain!");
    }

    this->hasTrustedHost = true;
    this->trustedHost = std::pair<string, uint64_t>(bestHost, bestLength);
    this->trustedWork = bestWork;
}

/*
    returns the total POW of trusted host
*/
uint64_t HostManager::getTrustedHostWork() {
    return this->trustedWork;
}

/*
    Returns N unique random hosts
*/
set<string> HostManager::sampleHosts(int count) {
    int numToPick = min(count, (int) this->hosts.size());
    set<string> sampledHosts;
    while(sampledHosts.size() < numToPick) {
        sampledHosts.insert(this->hosts[rand()%this->hosts.size()]);
    }
    return sampledHosts;
}


/*
    Adds a peer to the host list, 
*/
void HostManager::addPeer(string addr) {
    // check if we already have this peer host
    auto existing = std::find(this->hosts.begin(), this->hosts.end(), addr);
    if (existing != this->hosts.end()) {
        return;
    } 

    // check if the host is reachable:
    try {
        string name = getName(addr);
    } catch(...) {
        // if not exit
        return;
    }
    
    // add to our host list
    this->hosts.push_back(addr);
    // pick random neighbor hosts and forward the addPeer request to them:
    set<string> neighbors = this->sampleHosts(ADD_PEER_BRANCH_FACTOR);
    vector<future<void>> reqs;
    for(auto neighbor : neighbors) {
        reqs.push_back(std::async([neighbor, addr](){
            if (neighbor == addr) return;
            try {
                addPeerNode(neighbor, addr);
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
    Returns the trusted hosts block hash for blockID
*/
SHA256Hash HostManager::getBlockHash(uint64_t blockId) {
    if (!this->hasTrustedHost) throw std::runtime_error("Cannot lookup block hashes without trusted host.");
    if (blockId > this->validationHashes.size() || blockId < 1) {
        return NULL_SHA256_HASH;
    } else {
        return this->validationHashes[blockId - 1];
    }
}

/*
    Downloads an initial list of peers and validates connectivity to them
*/
void HostManager::refreshHostList() {
    if (this->hostSources.size() == 0) return;
    
    set<string> fullHostList;

    // Iterate through all host sources merging into a combined peer list
    for (int i = 0; i < this->hostSources.size(); i++) {
        try {
            string hostUrl = this->hostSources[i];
            http::Request request{hostUrl};
            const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
            json hostList = json::parse(std::string{response.body.begin(), response.body.end()});
            for(auto host : hostList) {
                fullHostList.insert(string(host));
            }
        } catch (...) {
            continue;
        }
    }

    if (fullHostList.size() == 0) return;

    // iterate through all listed peer hosts
    for(auto hostJson : fullHostList) {
        // if we've already added this host skip
        string hostUrl = string(hostJson);
        auto existing = std::find(this->hosts.begin(), this->hosts.end(), hostUrl);
        if (existing != this->hosts.end()) continue;

        // otherwise try connecting to the host to confirm it's up
        try {
            
            string hostName = getName(hostUrl);
            this->hosts.push_back(hostUrl);
            Logger::logStatus(GREEN + "[ CONNECTED ] " + RESET  + hostUrl);
            
        } catch (...) {
            Logger::logStatus(RED + "[ UNREACHABLE ] " + RESET  + hostUrl);
        }
    }
}


/*
    Returns a list of all peer hosts
*/
vector<string> HostManager::getHosts(bool includeSelf) {
    vector<string> ret = this->hosts;
    if (includeSelf) ret.push_back(this->myAddress);
    return ret;
}

size_t HostManager::size() {
    return this->hosts.size();
}

/*
    Returns the current trusted host
*/
std::pair<string, uint64_t> HostManager::getTrustedHost() {
    if (!this->hasTrustedHost) this->initTrustedHost();
    return this->trustedHost;
}

/*
    Returns a random host
*/
std::pair<string,uint64_t> HostManager::getRandomHost() {
    set<string> hosts = this->sampleHosts(1);
    string host = *hosts.begin();
    uint64_t chainLength = getCurrentBlockCount(host);
    return std::pair<string, uint64_t>(host, chainLength);
}