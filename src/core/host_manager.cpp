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

        // if our node is in the host list, remove ourselves:        
        this->hosts.clear();
        vector<future<void>> reqs;
        for(auto host : hostList) {
            try {
                if (myName == "") {
                    this->hosts.push_back(string(host));
                    Logger::logStatus("Adding host: " + string(host));
                }else {
                    HostManager& hm = *this;
                    reqs.push_back(std::async([&host, &hm](){
                        string hostName = getName(host);
                        if (hostName != hm.myName) {
                            Logger::logStatus("Adding host: " + string(host));
                            hm.hosts.push_back(string(host));
                        }
                    }));
                }
            } catch (...) {
                Logger::logStatus("Host did not respond: " + string(host));
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
        reqs.push_back(std::async([&host, &bestHosts, &bestWork, &lock]() {
            try {
                uint64_t curr = getTotalWork(host);
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
                continue;
            }
        }));
        // block until all requests finish
        for(int i = 0; i < reqs.size(); i++) {
            reqs[i].get();
        }
    }
    if (bestHosts.size() == 0) throw std::runtime_error("Could not get chain length from any host");
    string bestHost = bestHosts[rand()%bestHosts.size()];
    return std::pair<string, uint64_t>(bestHost, bestWork);
}