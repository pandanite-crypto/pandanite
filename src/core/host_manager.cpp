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
        this->refreshHostList();
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

void HostManager::addPeer(string addr) {
    // check if we already have this peer host
    auto existing = std::find(this->hosts.begin(), this->hosts.end(), addr);
    if (existing != this->hosts.end()) {
        return;
    } 

    // add it
    this->hosts.push_back(addr);
    
    // pick random neighbor hosts and forward the addPeer request to them:
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


void HostManager::refreshHostList() {
    if (this->hostSources.size() == 0) return;
    
    set<string> fullHostList;
    try {
        // Iterate through all host sources until we find one that gives us
        // an initial peer list
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
            // check if we've already added this host
            string hostUrl = string(hostJson);
            auto existing = std::find(this->hosts.begin(), this->hosts.end(), hostUrl);
            if (existing != this->hosts.end()) continue;

            try {
                // if we are a miner (name=""), just add the host
                if (myName == "") {
                    this->hosts.push_back(hostUrl);
                    Logger::logStatus("Adding host: " + hostUrl);
                } else {
                    // otherwise, check if the host is live (send name request)
                    // if live, submit ourselves as a peer to that host.
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

std::pair<string, uint64_t> HostManager::getRandomHost() {
    // return a random host along with it's chain length
    string host = *this->sampleHosts(1).begin();
    uint64_t len = getCurrentBlockCount(host);
    return std::pair<string, uint64_t> (host, len);
}