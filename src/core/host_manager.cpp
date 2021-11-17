#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "../external/http.hpp"
#include <iostream>
using namespace std;

HostManager::HostManager(json config) {
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
    this->hosts.push_back("http://localhost:3000");
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
        string myIp = exec("curl -s http://checkip.amazonaws.com");
        
        this->hosts.clear();
        for(auto host : hostList) {
            string hostIp = exec("dig +short " + host.substr(7,s.length()-12))
            if (hostIp != myIp) {
                Logger::logStatus("Adding host: " + string(host));
                this->hosts.push_back(string(host));
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
    string bestHost = "";
    int bestCount = 0;
    for(auto host : this->hosts) {
        try {
            int curr = getCurrentBlockCount(host);
            if (curr > bestCount) {
                bestCount = curr;
                bestHost = host;
            }
        } catch (std::exception & e) {
            continue;
        }
    }
    if (bestHost == "") throw std::runtime_error("Could not get chain length from any host");
    return std::pair<string, int>(bestHost, bestCount);
}