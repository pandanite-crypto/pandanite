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
#define NUM_PEER_PROPAGATE_HOSTS 10
#define NUM_BLOCK_PROPAGATE_HOSTS 10

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


void HostManager::propagateBlock(Block& b) {
    // propagate the block to N randomly chosen peers
    set<string> sampledHosts = this->sampleHosts(NUM_BLOCK_PROPAGATE_HOSTS);
    vector<future<void>> reqs;
    for(auto peer : sampledHosts) {
        reqs.push_back(std::async([&peer, &b]() {
            submitBlock(peer, b);
        }));
    }

    for(int i = 0; i < reqs.size(); i++) {
        reqs[i].get();
    }
}

void HostManager::addPeer(string host) {
    if (this->peers.find(host) != this->peers.end()) return;
    this->peers.insert(host);
    // propagate the peer to N randomly chosen peers
    set<string> sampledHosts = this->sampleHosts(NUM_PEER_PROPAGATE_HOSTS);
}

void HostManager::refreshHostList() {
    if (this->hostSources.size() == 0) {
        this->initBestHost();
        return;
    }
    set<string> hostList;
    try {
        for (int i = 0; i < this->hostSources.size(); i++) {
            try {
                string hostUrl = this->hostSources[i];
                http::Request request{hostUrl};
                const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
                auto peers = json::parse(std::string{response.body.begin(), response.body.end()});
                for(auto h : peers) {
                    hostList.insert(h);
                }
            } catch (...) {
                continue;
            }
        }
        if (hostList.size() == 0) throw std::runtime_error("Could not fetch host directory.");
        this->hosts.clear();
        vector<future<void>> reqs;
        for(auto host : hostList) {
            if (this->isBanned(host)) continue;
            try {
                if (myName == "") {
                    this->hosts.push_back(string(host));
                    Logger::logStatus("Adding host: " + string(host));
                }else {
                    // send name request to check responsiveness of each node:
                    HostManager& hm = *this;
                    reqs.push_back(std::async([&host, &hm](){
                        string hostName = getName(host);
                        if (hostName != hm.myName) {
                            Logger::logStatus("Adding host: " + string(host));
                            hm.hosts.insert(string(host));
                            string myAddr = "http://" + exec("curl ifconfig.co") + ":3000";
                            addPeer(host, myAddr);
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
    this->findBestHost();
}

vector<string> HostManager::getHosts() {
    vector<string> allHosts;
    for(auto h : this->hosts) allHosts.push_back(h);
    return allHosts;
}

size_t HostManager::size() {
    return this->hosts.size();
}

uint64_t HostManager::downloadAndVerifyChain(string host) {
    vector<BlockHeader> chain;
    SHA256Hash lastHash = NULL_SHA256_HASH;
    uint64_t totalWork = 0;
    uint32_t numBlocks = 0;
    bool error = false;
    try {
        readBlockHeaders(host, [&totalWork, &chain, &lastHash, &error](BlockHeader blockHeader) {
            vector<Transaction> empty;
            Block block(blockHeader, empty);
            if (!block.verifyNonce()) error = true;
            if (block.getLastBlockHash() != lastHash) error = true;
            lastHash = block.getHash();
            totalWork+= block.getDifficulty();
            numBlocks = block.getId();
        });
        if (error) return 0;
        
        this->bestChainLength = numBlocks;
        return totalWork;
    } catch (...) {
        return 0;
    }
}

uint32_t HostManager::getBestChainLength() {
    return this->bestChainLength;
}

string HostManager::getBestHost() {
    return this->bestHost;
}


set<string> HostManager::sampleHosts(uint32_t numToPick) {
    numToPick = min(numToPick, this->hosts.size());
    while(sampledHosts.size() < numToPick) {
        sampledHosts.insert(this->hosts[rand() % this->hosts.size()]);
    }
    return sampledHosts;
}


void HostManager::findBestHost() {
    // TODO: make this asynchronous
    vector<future<void>> reqs;
    std::mutex lock;
    
    uint64_t mostWork = 0;
    string mostWorkHost = "";
    
    set<string> sampledHosts = this->sampleHosts(NUM_SEED_HOSTS);

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
    Logger::logStatus("findBestHost: " + mostWorkHost);
    this->bestHost = mostWorkHost;
}