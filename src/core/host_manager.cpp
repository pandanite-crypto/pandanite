#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "../external/http.hpp"
#include <iostream>
using namespace std;

HostManager::HostManager(json config) {
    json hostList;
    for (int i = 0; i < config["nodeSources"].size(); i++) {
        bool foundHost = false;
        try {
            string hostUrl = config["nodeSources"][i];
            http::Request request{hostUrl};
            const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
            hostList = json::parse(std::string{response.body.begin(), response.body.end()});
            foundHost = true;
            break;
        } catch (...) {
            continue;
        }
        if (!foundHost) throw std::runtime_error("Could not fetch node directory.");
    }

    // if our node is in the host list, remove ourselves:
    string myUrl = "http://" + exec("host $(curl -s http://checkip.amazonaws.com) | tail -c 51 | head -c 49") + ":3000";
    for(auto host : hostList) {
        if (host != myUrl) {
            cout<<"Adding host: "<<host<<endl;
            this->hosts.push_back(string(host));
        }
    }
}

HostManager::HostManager() {

}

size_t HostManager::size() {
    return this->hosts.size();
}

std::pair<string, int> HostManager::getLongestChainHost() {
    string bestHost = "";
    int bestCount = 0;
    for(auto host : this->hosts) {
        int curr = getCurrentBlockCount(host);
        if (curr > bestCount) {
            bestCount = curr;
            bestHost = host;
        }
    }
    if (bestHost == "") throw std::runtime_error("Could not get chain length from any host");
    return std::pair<string, int>(bestHost, bestCount);
}