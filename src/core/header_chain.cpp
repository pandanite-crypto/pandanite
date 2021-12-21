
#include "header_chain.hpp"
#include "api.hpp"
#include "logger.hpp"

#define LOCAL_CHAIN_CACHE 2500

void sync_headers(HeaderChain& chain) {
    while(true) {
        uint32_t targetBlockCount;
        try {
            targetBlockCount = getCurrentBlockCount(chain.getHostAddress());
        } catch(...) {
            Logger::logStatus("Could not connect to host=" + chain.getHostAddress());
        }
        
        uint32_t startCount = chain.numBlocks;

        if (startCount >= targetBlockCount) {
            Logger::logStatus("Chain from host " + chain.host + " in sync, length=" +to_string(chain.getChainLength()) + " totalWork=" + to_string(chain.getTotalWork()));
            continue;
        }

        int needed = targetBlockCount - startCount;
        Logger::logStatus("Downloading " + to_string(needed) + " block headers from " + chain.host);
        for(int i = startCount + 1; i <= targetBlockCount; i+=BLOCKS_PER_HEADER_FETCH) {
            int end = min((int) targetBlockCount, (int) i + BLOCKS_PER_HEADER_FETCH - 1);
            bool error = false;
            try {
                cout<<"reading " << i << ","<<end <<endl;
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
                // pop off 50 blocks and see if we can
                for(int j = 0; j < 50; j++) {
                    if (chain.headers.size() == 0) {
                        chain.resetChain();
                    } else {
                        chain.popBlock();
                    }
                }
            }
            while(chain.headers.size() > LOCAL_CHAIN_CACHE) {
                chain.discardBlock();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


HeaderChain::HeaderChain(string host) {
    this->host = host;
    this->resetChain();
    this->syncThread.push_back(std::thread(sync_headers, ref(*this)));
}

string HeaderChain::getHostAddress() {
    return this->host;
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