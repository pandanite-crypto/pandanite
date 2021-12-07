#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "../external/http.hpp"
#include <iostream>
#include <thread>
#include <future>
#include <set>
using namespace std;

#define NUM_SEED_HOSTS 10

HostManager::HostManager(json config, string myName) {
    this->myName = myName;
    for(auto h : config["hostSources"]) {
        this->hostSources.push_back(h);
    }

    if (this->hostSources.size() == 0) {
        for(auto h : config["hosts"]) {
            this->hosts.push_back(h);
        }
    }     
    this->refreshHostList();
}

HostManager::HostManager() {
}

void HostManager::banHost(string host) {
    this->bannedHosts.insert(host);
}

bool HostManager::isBanned(string host) {
    return this->bannedHosts.find(host) != this->bannedHosts.end();
}

void HostManager::refreshHostList() {
    if (this->hostSources.size() == 0) {
        this->initBestHost();
        return;
    }
    
    json hostList;
    try {
        bool foundHost = false;
        for (int i = 0; i < this->hostSources.size(); i++) {
            try {
                string hostUrl = this->hostSources[i];
                http::Request request{hostUrl};
                const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
                // TODO : call /peers on each and merge into set
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
                        if (hostName != hm.myName && !hm.isBanned(host)) {
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
    this->initBestHost();
}

vector<string> HostManager::getHosts() {
    return this->hosts;
}

size_t HostManager::size() {
    return this->hosts.size();
}

uint64_t HostManager::downloadAndVerifyChain(string host) {
    vector<BlockHeader> chain;
    SHA256Hash lastHash = NULL_SHA256_HASH;
    uint64_t totalWork = 0;
    bool error = false;
    try {
        readBlockHeaders(host, [&totalWork, &chain, &lastHash, &error](BlockHeader blockHeader) {
            vector<Transaction> empty;
            Block block(blockHeader, empty);
            Logger::logStatus("verifying block header " + to_string(block.getId()));
            if (!block.verifyNonce()) error = true;
            if (block.getLastBlockHash() != lastHash) error = true;
            lastHash = block.getHash();
            totalWork+= block.getDifficulty();
        });
        if (error) return 0;
        return totalWork;
    } catch (...) {
        return 0;
    }
}

string HostManager::getBestHost() {
    return this->bestHost;
}

void HostManager::initBestHost() {
    // TODO: make this asynchronous
    vector<future<void>> reqs;
    std::mutex lock;
    set<string> sampledHosts;
    uint64_t mostWork = 0;
    string mostWorkHost = "";
    
    int numToPick = min((size_t)NUM_SEED_HOSTS, this->hosts.size());

    while(sampledHosts.size() < numToPick) {
        sampledHosts.insert(this->hosts[rand() % this->hosts.size()]);
    }
    for (auto host : sampledHosts) {
        HostManager& hm = *this;
        reqs.push_back(std::async([&host, &hm, &lock, &mostWork, &mostWorkHost](){
            uint64_t totalWork = hm.downloadAndVerifyChain(host);
            lock.lock();
            if (totalWork > mostWork) {
                mostWork = totalWork;
                mostWorkHost = host;
            }    
            lock.unlock();
        }));
    }

    for(int i = 0; i < reqs.size(); i++) {
        reqs[i].get();
    }
    Logger::logStatus("initBestHost: " + mostWorkHost);
    this->bestHost = mostWorkHost;
}