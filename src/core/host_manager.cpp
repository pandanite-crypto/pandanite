#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "../external/http.hpp"
#include <iostream>
#include <thread>
#include <future>
using namespace std;

#define ADD_PEER_BRANCH_FACTOR 10

HostManager::HostManager(json config, string myName) {
    this->myName = myName;
    for(auto h : config["hostSources"]) {
        this->hostSources.push_back(h);
    }
    if (this->hostSources.size() == 0) {
        this->hosts.push_back("http://localhost:3000");
    } else {
        this->refreshHostList();
    }
}

HostManager::HostManager() {
}

string computeAddress() {
    string rawUrl = exec("curl ifconfig.co") ;
    return "http://" + rawUrl.substr(0, rawUrl.size() - 1) + ":3000";
}

set<string> HostManager::sampleHosts(int count) {
    int numToPick = min(count, this->hosts.size());
    set<string> sampledHosts;
    while(sampledHosts.size() < numToPick) {
        sampledHosts.insert(this->hosts[rand()%this->hosts.size()]);
    }
    return sampledHosts;
}

void HostManager::addPeer(string addr) {
    auto existing = std::find(this->hosts.begin(), this->hosts.end(), addr);
    if (existing != this->hosts.end()) return;

    // check if host is reachable:
    try {   
        getName(addr);
    } catch (...) {
        return;
    }

    this->hosts.push_back(addr);
    
    // pick random hosts and add send cascade:
    set<string> neighbors = this->sampleHosts(ADD_PEER_BRANCH_FACTOR);
    vector<future<void>> reqs;
    for(auto neighbor : neighbors) {
        reqs.push_back(std::async([neighbor, addr](){
            try {
                addPeer(neighbor, addr);
            } catch(...) {
                Logger::logStatus("Could not add peer " + addr + " to " + neighbor);
            }
        }));
    }

    for(int i =0 ; i < reqs.size(); i++) {
        reqs[i].get();
    }
}

void HostManager::refreshHostList() {
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
                            Logger::logStatus("Failed to load name");
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

    } catch (std::exception &e) {
        Logger::logError("HostManager::refreshHostList", string(e.what()));
    }
}

vector<string> HostManager::getHosts() {
    return this->hosts;
}

size_t HostManager::size() {
    return this->hosts.size();
}

std::pair<string, uint64_t> HostManager::getBestHost() {
    // TODO: make this asynchronous
    uint64_t bestWork = 0;
    vector<string> bestHosts;
    vector<future<void>> reqs;
    std::mutex lock;
    for(auto host : this->hosts) {
        reqs.push_back(std::async([host, &bestHosts, &bestWork, &lock]() {
            try {
                uint64_t curr = getCurrentBlockCount(host); //getTotalWork(host);
                lock.lock();
                if (curr > bestWork) {
                    bestWork = curr;
                    bestHosts.clear();
                    bestHosts.push_back(host);
                } else if (curr == bestWork) {
                    bestHosts.push_back(host);
                }
                lock.unlock();
            } catch (std::exception & e) {
                lock.unlock();
            }
        }));
    }    
    // block until all requests finish
    for(int i = 0; i < reqs.size(); i++) {
        reqs[i].get();
    }

    if (bestHosts.size() == 0) throw std::runtime_error("Could not get chain length from any host");
    string bestHost = bestHosts[rand()%bestHosts.size()];
    return std::pair<string, uint64_t>(bestHost, bestWork);
}