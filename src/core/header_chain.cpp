#include "header_chain.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "common.hpp"

void HeaderChain::HeaderChain(string host) {
    this->host = host;
}



void HeaderChain::load() {
    uint64_t targetBlockCount = getCurrentBlockCount(this->host);
    SHA256Hash lastHash = NULL_SHA256_HASH;
    // download any remaining blocks in batches
    for(int i = 1; i <= targetBlockCount; i+=BLOCK_HEADERS_PER_FETCH) {
        try {
            int end = min(targetBlockCount, i + BLOCK_HEADERS_PER_FETCH - 1);
            bool failure = false;
            readRawHeaders(this->host, i, end, [&failure](BlockHeader& b) {
                
            });
            if (failure) {
                this->failed = true;
                return;
            }
        } catch (...) {
            this->failed = true;
            return;
        }
    }
}


