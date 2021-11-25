#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "../external/http.hpp"
#include <iostream>
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
        for (int i = 0; i < this->hostSources.size(); i++) {
            bool foundHost = false;
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
            if (!foundHost) throw std::runtime_error("Could not fetch host directory.");
        }

        // if our node is in the host list, remove ourselves:        
        this->hosts.clear();
        for(auto host : hostList) {
            try {
                if (myName == "") {
                    this->hosts.push_back(string(host));
                    Logger::logStatus("Adding host: " + string(host));
                }else {
                    string hostName = getName(host);
                    if (hostName != this->myName) {
                        Logger::logStatus("Adding host: " + string(host));
                        this->hosts.push_back(string(host));
                    }
                }
            } catch (...) {
                Logger::logStatus("Host did not respond: " + string(host));
            }
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

std::pair<string, int> HostManager::getLongestChainHost() {
    // TODO: make this asynchronous
    int bestCount = 0;
    vector<string> bestHosts;
    for(auto host : this->hosts) {
        try {
            int curr = getCurrentBlockCount(host);
            if (curr > bestCount) {
                bestCount = curr;
                bestHosts.clear();
                bestHosts.push_back(host);
            } else if (curr == bestCount) {
                bestHosts.push_back(host);
            }
        } catch (std::exception & e) {
            continue;
        }
    }
    if (bestHosts.size() == 0) throw std::runtime_error("Could not get chain length from any host");
    string bestHost = bestHosts[rand()%bestHosts.size()];
    return std::pair<string, int>(bestHost, bestCount);
}