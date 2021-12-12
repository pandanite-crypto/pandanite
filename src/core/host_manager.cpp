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

void HostManager::addPeer(string addr) {
    auto existing = std::find(this->hosts.begin(), this->hosts.end(), addr);
    if (existing != this->hosts.end()) return;
    this->hosts.push_back(addr);
}

void HostManager::refreshHostList() {
    if (this->hostSources.size() == 0) return;
    json hostList;
    try {
        bool foundHost = false;
        for (int i = 0; i < this->hostSources.size(); i++) {
            try {
                string hostUrl = this->hostSources[i];
                http::Request request{hostUrl};
                const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
                hostList = json::parse(std::string{response.body.begin(), response.body.end()});
                foundHost = true;
                break;
            } catch (...) {
                continue;
            }
        }
        if (!foundHost) throw std::runtime_error("Could not fetch host directory.");
        
        vector<future<void>> reqs;
        for(auto hostJson : hostList) {
            string hostUrl = string(hostJson);
            auto existing = std::find(this->hosts.begin(), this->hosts.end(), hostUrl);
            if (existing != this->hosts.end()) continue;
            try {
                if (myName == "") {
                    this->hosts.push_back(hostUrl);
                    Logger::logStatus("Adding host: " + hostUrl);
                }else {
                    HostManager& hm = *this;
                    reqs.push_back(std::async([&hostUrl, &hm](){
                        string hostName = getName(hostUrl);
                        if (hostName != hm.myName) {
                            Logger::logStatus("Adding host: " + hostUrl);
                            hm.hosts.push_back(hostUrl);
                            // add self as peer to host:
                            try {
                                string myAddress = computeAddress();
                                addPeerNode(hostUrl, myAddress);
                            } catch(...) {
                                Logger::logStatus("Failed to register self as peer to " + hostUrl);
                            }
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