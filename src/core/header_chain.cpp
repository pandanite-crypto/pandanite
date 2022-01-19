#include "header_chain.hpp"
#include "api.hpp"
#include "block.hpp"
#include "logger.hpp"
#include <iostream>
using namespace std;

HeaderChain::HeaderChain(string host) {
    this->host = host;
    this->failed = false;
    this->totalWork = 0;
    this->chainLength = 0;
}

bool HeaderChain::valid() {
    return !this->failed && this->totalWork > 0;
}

string HeaderChain::getHost() {
    return this->host;
}

Bigint HeaderChain::getTotalWork() {
    if (this->failed) return 0;
    return this->totalWork;
}
uint64_t HeaderChain::getChainLength() {
    if (this->failed) return 0;
    return this->chainLength;
}

void HeaderChain::load() {
    uint64_t targetBlockCount;
    try {
        targetBlockCount = getCurrentBlockCount(this->host);
    } catch (...) {
        this->failed = true;
        return;
    }
    SHA256Hash lastHash = NULL_SHA256_HASH;
    uint64_t numBlocks = 0;
    Bigint totalWork = 0;
    // download any remaining blocks in batches
    for(int i = 1; i <= targetBlockCount; i+=BLOCK_HEADERS_PER_FETCH) {
        try {
            int end = min(targetBlockCount, (uint64_t) i + BLOCK_HEADERS_PER_FETCH - 1);
            bool failure = false;
            vector<SHA256Hash>& hashes = this->blockHashes;
            vector<BlockHeader> blockHeaders;
            readRawHeaders(this->host, i, end, blockHeaders);
            for (auto& b : blockHeaders) {
                if (failure) return;
                vector<Transaction> empty;
                Block block(b, empty);
                if (!block.verifyNonce()) {
                    failure = true;
                    break;
                };
                if (block.getLastBlockHash() != lastHash) {
                    failure = true;
                    break;
                }
                lastHash = block.getHash();
                hashes.push_back(lastHash);
                totalWork = addWork(totalWork, block.getDifficulty());
                numBlocks++;
            }
            if (failure) {
                this->failed = true;
                return;
            }
        } catch (std::exception& e) {
            this->failed = true;
            return;
        } catch (...) {
            this->failed = true;
            return;
        }
    }
    this->chainLength = numBlocks;
    this->totalWork = totalWork;
    this->failed = false;
}


