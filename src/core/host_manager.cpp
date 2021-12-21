#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "header_chain.hpp"
#include "../external/http.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <future>
using namespace std;

#define ADD_PEER_BRANCH_FACTOR 10
#define NUM_RANDOM_HOSTS 3

string computeAddress() {
    string rawUrl = exec("curl -s ifconfig.co") ;
    return "http://" + rawUrl.substr(0, rawUrl.size() - 1) + ":3000";
}

HostManager::HostManager(json config, string myName) {
    this->myName = myName;
    this->myAddress = computeAddress();
    this->disabled = false;
    for(auto h : config["hostSources"]) {
        this->hostSources.push_back(h);
    }
    if (this->hostSources.size() == 0) {
        this->hosts.push_back("http://localhost:3000");
    } else {
        this->refreshHostList(true);
    }
}

HostManager::HostManager() {
    this->disabled = true;
}

bool HostManager::isDisabled() {
    return this->disabled;
}

set<string> HostManager::sampleHosts(int count) {
    int numToPick = min(count, (int) this->hosts.size());
    set<string> sampledHosts;
    while(sampledHosts.size() < numToPick) {
        sampledHosts.insert(this->hosts[rand()%this->hosts.size()]);
    }
    return sampledHosts;
}

string HostManager::getUnusedHost() {
    // create a set of all hosts
    set<string> all;
    for(auto h : this->hosts) all.insert(h);
    set<string> curr;
    for(auto h : this->currentHosts) curr.insert(h->getHostAddress());

    std::set<string> result;
    std::set_difference(all.begin(), all.end(), curr.begin(), curr.end(), std::inserter(result, result.end()));

    if (result.size() == 0) return "";
    
    auto it = result.begin();
    return *it;
}

void HostManager::addPeer(string addr) {
    auto existing = std::find(this->hosts.begin(), this->hosts.end(), addr);
    if (existing != this->hosts.end()) {
        Logger::logStatus("Host already in list");
        return;
    } 

    this->hosts.push_back(addr);
    
    // pick random hosts and add send cascade:
    set<string> neighbors = this->sampleHosts(ADD_PEER_BRANCH_FACTOR);
    vector<future<void>> reqs;
    for(auto neighbor : neighbors) {
        reqs.push_back(std::async([neighbor, addr](){
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

void HostManager::refreshHostList(bool resampleHeadChains) {
    if (this->hostSources.size() == 0) return;
    
    set<string> fullHostList;
    try {
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
        if (fullHostList.size() == 0) throw std::runtime_error("Could not fetch host directory.");
        
        vector<future<void>> reqs;
        for(auto hostJson : fullHostList) {
            string hostUrl = string(hostJson);
            auto existing = std::find(this->hosts.begin(), this->hosts.end(), hostUrl);
            if (existing != this->hosts.end()) continue;
            try {
                if (myName == "") {
                    this->hosts.push_back(hostUrl);
                    Logger::logStatus("Adding host: " + hostUrl);
                } else {
                    HostManager& hm = *this;
                    reqs.push_back(std::async([hostUrl, &hm](){
                        try {
                            Logger::logStatus("Adding host: " + hostUrl);
                            string hostName = getName(hostUrl);
                            if (hostName != hm.myName) {
                                hm.hosts.push_back(hostUrl);
                                // add self as peer to host:
                                try {
                                    string myAddress = computeAddress();
                                    addPeerNode(hostUrl, myAddress);
                                    Logger::logStatus("Added " + myAddress + " as peer to " + hostUrl);
                                } catch(...) {
                                    Logger::logStatus("Failed to register self as peer to " + hostUrl);
                                }
                            }
                        } catch (...) {
                            Logger::logStatus("Host did not respond: " + hostUrl);
                        }
                    }));
                }
            } catch (...) {
                Logger::logStatus("Host did not respond: " + hostUrl);
            }
        }
        // block until all requests finish or timeout
        for(int i = 0; i < reqs.size(); i++) {
            reqs[i].get();
        }
        
        if (resampleHeadChains) {
            set<string> headChains = this->sampleHosts(NUM_RANDOM_HOSTS);
            for(auto h : headChains) {
                this->currentHosts.push_back(new HeaderChain(h, this));
            }
        }

    } catch (std::exception &e) {
        Logger::logError("HostManager::refreshHostList", string(e.what()));
    }
}

vector<string> HostManager::getHosts(bool includeSelf) {
    vector<string> ret = this->hosts;
    if (includeSelf) ret.push_back(this->myAddress);
    return ret;
}

size_t HostManager::size() {
    return this->hosts.size();
}

bool HostManager::isReady() {
    for (auto host : this->currentHosts) {
        if (host->isReady()) return true;
    }
    return false;
}

std::pair<string, uint64_t> HostManager::getBestHost() {
    // TODO: make this asynchronous
    uint64_t bestWork = 0;
    uint64_t bestChainLength = 0;
    vector<string> bestHostAddresses;
    std::mutex lock;
    for(int i = 0; i < this->currentHosts.size(); i++) {
        HeaderChain* host = this->currentHosts[i];
        uint64_t curr = host->getTotalWork();
        if (curr > bestWork) {
            bestHostAddresses.clear();
            bestWork = curr;
            bestChainLength = host->getChainLength();
            bestHostAddresses.push_back(host->getHostAddress());
        } else if (curr == bestWork) {
            bestHostAddresses.push_back(host->getHostAddress());
        }
    }    
    if (bestChainLength == 0) throw std::runtime_error("Could not get chain length from any host");
    
    string bestHost = bestHostAddresses[rand()%bestHostAddresses.size()];
    return std::pair<string, uint64_t>(bestHost, bestChainLength);
}