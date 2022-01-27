#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
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
#define HEADER_VALIDATION_HOST_COUNT 8
#define RANDOM_GOOD_HOST_COUNT 9
#define HOST_MIN_FRESHNESS 180 * 60 // 3 hours

/*
    Fetches the public IP of the node
*/

bool isValidIPv4(string & ip) {
   unsigned int a,b,c,d;
   return sscanf(ip.c_str(),"%d.%d.%d.%d", &a, &b, &c, &d) == 4;
}

string HostManager::computeAddress() {
    if (this->ip == "") {
        bool found = false;
        vector<string> lookupServices = { "checkip.amazonaws.com", "icanhazip.com", "ifconfig.co", "wtfismyip.com/text", "ifconfig.io" };

        for(auto& lookupService : lookupServices) {
            string cmd = "curl -s4 " + lookupService;
            string rawUrl = exec(cmd.c_str());
            string ip = rawUrl.substr(0, rawUrl.size() - 1);
            if (isValidIPv4(ip)) {
                this->address = "http://" + ip  + ":" + to_string(this->port);
                found = true;
                break;
            }
        }

        if (!found) {
            Logger::logError("IP discovery", "Could not determine current IP address");
        }
    } else {
        this->address = this->ip + ":" + to_string(this->port);
    }
    return this->address;
}

/*
    This thread periodically updates all neighboring hosts with the 
    nodes current IP 
*/  
void peer_sync(HostManager& hm) {
    while(true) {
        for(auto host : hm.hosts) {
            try {
                pingPeer(host, hm.computeAddress(), std::time(0), hm.version);
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
    this->computeAddress();

    // check if a blacklist file exists
    std::ifstream blacklist("blacklist.txt");
    if (blacklist.good()) {
        std::string line;
        while (std::getline(blacklist, line)) {
            if (line[0] != '#') {
                this->blacklist.insert(line);
                Logger::logStatus("Ignoring host " + line);
            }
        }
    }

    this->disabled = false;
    this->hasTrustedHost = false;
    this->trustedWork = 0;
    for(auto h : config["hostSources"]) {
        this->hostSources.push_back(h);
    }
    if (this->hostSources.size() == 0) {
        string localhost = "http://localhost:3000";
        this->hosts.push_back(localhost);
        this->hostPingTimes[localhost] = std::time(0);
        this->peerClockDeltas[localhost] = 0;
    } else {
        this->refreshHostList();
    }
    
}

void HostManager::startPingingPeers() {
    if (this->syncThread.size() > 0) throw std::runtime_error("Peer ping thread exists.");
    this->syncThread.push_back(std::thread(peer_sync, ref(*this)));
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
        // only use peers that have pinged in the last hour
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
    Asks nodes for their current POW and chooses the best
*/
string HostManager::getGoodHost() {
    // pick random hosts
    set<string> hosts = this->sampleAllHosts(RANDOM_GOOD_HOST_COUNT);

    if (hosts.size() == 0) {
        Logger::logStatus("No hosts found");
    }

    Logger::logStatus("Getting chain lengths from peers (finding current best host).");

    vector<std::pair<string,Bigint>> chains;
    // TODO: these should happen asynchronously!
    for(auto host : hosts) {
        try {
            Bigint work = getTotalWork(host);
            chains.push_back(std::pair<string,Bigint>(host, work));
        } catch (...) {
            // host unreachable
        }
    }
    // pick the best (highest POW) node as host:
    Bigint bestWork = 0;
    string bestHost = "";
    
    for(auto chain : chains) {
        if (chain.second > bestWork) {
            bestWork = chain.second;
            bestHost = chain.first;
        }
    }
    Logger::logStatus("Got host : " + bestHost);
    return bestHost;
}

/*
    Loads header chains from peers and validates them, storing hash headers from highest POW chain
*/
void HostManager::initTrustedHost() {
    // pick random hosts
    set<string> hosts = this->sampleFreshHosts(HEADER_VALIDATION_HOST_COUNT);
    if (hosts.size() == 0) {
        Logger::logStatus("No hosts found");
    }

    vector<HeaderChain*> chains;

    // fetch block headers from each host
    vector<std::thread> threads;
    Logger::logStatus("Validating header chains from peers [This may take 15-20 minutes]");

    for(auto& host : hosts) {
        HeaderChain * newChain = new HeaderChain(host);
        chains.push_back(newChain);
        Logger::logStatus(">> Loading headers from " + host);
        threads.emplace_back(
            std::thread([newChain](){
                newChain->load();
            })
        );
    }

    for (auto& th : threads) th.join();

    for(auto& chain : chains) {
        Logger::logStatus(">> Loaded " + to_string(chain->getChainLength()) + " blocks from " + chain->getHost());
    }

    // pick the best (highest POW) header chain as host:
    Bigint bestWork = 0;
    uint64_t bestLength = 0;
    string bestHost = "";
    
    for(auto chain : chains) {
        if (chain->valid()) {
            if (chain->getTotalWork() > bestWork) {
                bestWork = chain->getTotalWork();
                bestLength = chain->getChainLength();
                bestHost = chain->getHost();
                // store the validation hashes 
                this->validationHashes = chain->blockHashes;
            }
        }
    }

    if (bestHost == "") {
        throw std::runtime_error("Could not find valid header chain!");
    }

    for(auto& chain : chains) {
        delete chain;
    }

    Logger::logStatus(GREEN + "[ FINISHED ]" + RESET);

    this->hasTrustedHost = true;
    this->trustedHost = std::pair<string, uint64_t>(bestHost, bestLength);
    this->trustedWork = bestWork;
}

/*
    returns the total POW of trusted host
*/
Bigint HostManager::getTrustedHostWork() {
    return this->trustedWork;
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
void HostManager::addPeer(string addr, uint64_t time, string version) {
    if (version != this->version) return;
    // check if peer is on our local blacklist
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
        string name = getName(addr);
    } catch(...) {
        // if not exit
        return;
    }
    
    // add to our host list
    this->hosts.push_back(addr);
    // pick random neighbor hosts and forward the addPeer request to them:
    set<string> neighbors = this->sampleFreshHosts(ADD_PEER_BRANCH_FACTOR);
    vector<future<void>> reqs;
    string _version = this->version;
    for(auto neighbor : neighbors) {
        reqs.push_back(std::async([neighbor, addr, _version](){
            if (neighbor == addr) return;
            try {
                pingPeer(neighbor, addr, std::time(0), _version);
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
    
    Logger::logStatus("Finding peers...");

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
    vector<std::thread> threads;
    std::mutex lock;
    for(auto hostJson : fullHostList) {
        // if we've already added this host skip
        string hostUrl = string(hostJson);
        auto existing = std::find(this->hosts.begin(), this->hosts.end(), hostUrl);
        if (existing != this->hosts.end()) continue;

        // if host is in blacklist skip:
        if (this->blacklist.find(hostUrl) != this->blacklist.end()) continue;

        // otherwise try connecting to the host to confirm it's up
        HostManager & hm = *this;
        threads.emplace_back(
            std::thread([hostUrl, &hm, &lock](){
                try {
                    string hostName = getName(hostUrl);
                    lock.lock();
                    hm.hosts.push_back(hostUrl);
                    hm.hostPingTimes[hostUrl] = std::time(0);
                    lock.unlock();
                    Logger::logStatus(GREEN + "[ CONNECTED ] " + RESET  + hostUrl);
                } catch (...) {
                    Logger::logStatus(RED + "[ UNREACHABLE ] " + RESET  + hostUrl);
                }
            })
        );
    }
    for (auto& th : threads) th.join();
}


/*
    Returns a list of all peer hosts
*/
vector<string> HostManager::getHosts(bool includeSelf) {
    vector<string> ret;
    for (auto pair : this->hostPingTimes) {
        uint64_t lastPingDelta = std::time(0) - pair.second;
        // only return peers that have pinged in the last hour
        if (lastPingDelta < HOST_MIN_FRESHNESS) { 
            ret.push_back(pair.first);
        }
    }
    if (includeSelf) ret.push_back(this->address);
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
    set<string> hosts = this->sampleAllHosts(1);
    if (hosts.size() == 0) {
        return std::pair<string, uint64_t>("", 0);
    }
    string host = *hosts.begin();
    uint64_t chainLength = getCurrentBlockCount(host);
    return std::pair<string, uint64_t>(host, chainLength);
}