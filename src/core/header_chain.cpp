
#include "header_chain.hpp"
#include "api.hpp"
#include "logger.hpp"
#include "host_manager.hpp"

#define LOCAL_CHAIN_CACHE 2500

void sync_headers(HeaderChain& chain) {
    uint64_t statusFailureCount = 0;
    uint64_t downloadFailureCount = 0;
    while(true) {
        uint32_t targetBlockCount;
        try {
            targetBlockCount = getCurrentBlockCount(chain.getHostAddress());
            statusFailureCount = 0;
        } catch(...) {
            statusFailureCount++;
        }

        if (statusFailureCount > 5 || downloadFailureCount > 10) {
            Logger::logStatus("Discarding host=" + chain.getHostAddress());
            string newPeer = chain.hostManager->getUnusedHost();
            if (newPeer == "") {
                // give up
                Logger::logStatus("No alternate peer hosts. Giving up.");
                return;
            } else {
                Logger::logStatus("Swapped non-responsive chain to new host=" + newPeer);
                chain.setHost(newPeer);
                downloadFailureCount = 0;
                statusFailureCount = 0;
            }
            continue;
        }
            
        uint32_t startCount = chain.numBlocks;

        if (startCount >= targetBlockCount) {
            if (!chain.ready) Logger::logStatus("Chain from host " + chain.host + " in sync, length=" +to_string(chain.getChainLength()) + " totalWork=" + to_string(chain.getTotalWork()));
            chain.ready = true;
            continue;
        }
        int needed = targetBlockCount - startCount;
        Logger::logStatus("Downloading " + to_string(needed) + " block headers from " + chain.host);
        for(int i = startCount + 1; i <= targetBlockCount; i+=BLOCK_HEADERS_PER_FETCH) {
            int end = min((int) targetBlockCount, (int) i + BLOCK_HEADERS_PER_FETCH - 1);
            bool error = false;
            try {
                readRawHeaders(chain.host, i, end, [&chain, &error](BlockHeader & blockHeader) {
                    vector<Transaction> empty;
                    Block block(blockHeader, empty);
                    if (!block.verifyNonce()) {
                        error = true;
                    };
                    if (block.getLastBlockHash() != chain.lastHash) {
                        error = true;
                    }
                    chain.lastHash = block.getHash();
                    chain.totalWork+= block.getDifficulty();
                    chain.numBlocks++;
                    return error;
                });
            } catch(const std::exception &e) {
                error = true;
                Logger::logStatus("Could not fetch block headers for host=" + chain.host);
                Logger::logStatus(e.what());
            } catch (...) {
                error = true;
                Logger::logStatus("Could not fetch block headers for host=" + chain.host);
            }
            if (error) {
                downloadFailureCount++;
                // pop off 50 blocks and see if we can re-sync
                for(int j = 0; j < 50; j++) {
                    if (chain.headers.size() == 0) {
                        chain.resetChain();
                        break;
                    } else {
                        chain.popBlock();
                    }
                }
            } else {
                downloadFailureCount = 0;
                statusFailureCount = 0;
            }
            while(chain.headers.size() > LOCAL_CHAIN_CACHE) {
                chain.discardBlock();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

HeaderChain::HeaderChain(string host, HostManager* hm) {
    this->host = host;
    this->resetChain();
    this->ready = false;
    this->hostManager = hm;
    this->syncThread.push_back(std::thread(sync_headers, ref(*this)));
}

void HeaderChain::setHost(string host) {
    this->host = host;
}

string HeaderChain::getHostAddress() {
    return this->host;
}

bool HeaderChain::isReady() {
    return this->ready;
}

void HeaderChain::resetChain() {
    this->totalWork = 0;
    this->numBlocks = 0;
    this->lastHash = NULL_SHA256_HASH;
    this->headers = list<BlockHeader>();
}

void HeaderChain::popBlock() {
    this->lastHash = this->headers.back().lastBlockHash;
    this->totalWork -= this->headers.back().difficulty;
    this->numBlocks--;
    this->headers.pop_back();
}

void HeaderChain::discardBlock() {
    this->headers.pop_front();
}

uint64_t HeaderChain::getTotalWork() {
    return this->totalWork;
};

uint64_t HeaderChain::getChainLength() {
    return this->numBlocks;
};