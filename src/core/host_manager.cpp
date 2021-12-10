#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "../external/http.hpp"
#include <iostream>
#include <thread>
#include <future>
#include <algorithm>
#include <random>
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
            this->hosts.insert(string(h));
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

void HostManager::addPeer(string host) {
    if (this->peers.find(host) != this->peers.end()) return;
    this->peers.insert(host);
    // propagate the peer to N randomly chosen peers
    vector<string> sampledHosts = this->sampleHosts(NUM_PEER_PROPAGATE_HOSTS);
    for (auto neighbor : sampledHosts) {
        addPeerNode(neighbor, host);
    }
}


void HostManager::propagateBlock(Block& b) {
    // propagate the block to N randomly chosen peers
    vector<string> sampledHosts = this->sampleHosts(NUM_BLOCK_PROPAGATE_HOSTS);
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

void HostManager::refreshHostList() {
    set<string> hostList;
    try {
        for (int i = 0; i < this->hostSources.size(); i++) {
            try {
                string hostUrl = this->hostSources[i];
                http::Request request{hostUrl};
                const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
                auto peers = json::parse(std::string{response.body.begin(), response.body.end()});
                for(auto h : peers) {
                    hostList.insert(string(h));
                }
            } catch (...) {
                continue;
            }
        }
        for(auto h : this->hosts) {
            hostList.insert(h);
        }
        this->hosts.clear();
        vector<future<void>> reqs;
        for(auto host : hostList) {
            if (this->isBanned(host)) continue;
            try {
                if (this->myName == "") {
                    this->hosts.insert(string(host));
                    Logger::logStatus("Adding host: " + string(host));
                }else {
                    HostManager& hm = *this;
                    string myAddr = "http://" + exec("curl ifconfig.co") + ":3000";
                    Logger::logStatus("My address is: " + myAddr);
                    reqs.push_back(std::async([&host, &myAddr, &hm](){
                        // send name request to check responsiveness of each node:
                        string hostName = getName(host);
                        if (hostName != hm.myName) {
                            Logger::logStatus("Adding peer host: " + string(host));
                            hm.hosts.insert(string(host));
                            // add ourself as a peer node of the host
                            addPeerNode(host, myAddr);
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
    for(auto h : this->peers) allHosts.push_back(h);
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
        readBlockHeaders(host, [&totalWork, &numBlocks, &chain, &lastHash, &error](BlockHeader blockHeader) {
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
        this->bestChainWork = totalWork;
        return totalWork;
    } catch (...) {
        return 0;
    }
}

uint32_t HostManager::getBestChainLength() {
    return this->bestChainLength;
}

uint64_t HostManager::getBestChainWork() {
    return this->bestChainWork;
}

string HostManager::getBestHost() {
    return this->bestHost;
}


vector<string> HostManager::sampleHosts(uint32_t numToPick) {
    vector<string> sampledHosts;
    numToPick = min(numToPick, (uint32_t) this->hosts.size());
    std::sample(this->hosts.begin(), this->hosts.end(), std::back_inserter(sampledHosts),
                numToPick, std::mt19937{std::random_device{}()});
    return std::move(sampledHosts);
}


void HostManager::findBestHost() {
    vector<future<void>> reqs;
    std::mutex lock;
    
    uint64_t mostWork = 0;
    string mostWorkHost = "";

    // pick random hosts:
    vector<string> sampledHosts = this->sampleHosts(NUM_SEED_HOSTS);

    // download and verify chain from each host
    for (auto host : sampledHosts) {
        HostManager& hm = *this;
        reqs.push_back(std::async([&host, &hm, &lock, &mostWork, &mostWorkHost](){
            uint64_t totalWork = hm.downloadAndVerifyChain(host);
            Logger::logStatus("Downloaded chain with totalWork=" + to_string(totalWork));
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