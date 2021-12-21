
#include "header_chain.hpp"
#include "api.hpp"

#define LOCAL_CHAIN_CACHE 2500

void sync_headers(HeaderChain& chain) {
    while(true) {
        uint32_t targetBlockCount = getCurrentBlockCount(chain.host);
        uint32_t startCount = chain.numBlocks;

        if (startCount >= targetBlockCount) {
            return SUCCESS;
        }

        int needed = targetBlockCount - startCount;
        for(int i = startCount + 1; i <= targetBlockCount; i+=BLOCKS_PER_HEADER_FETCH) {
            bool error = false;
            readRawHeaders(i, i + BLOCKS_PER_HEADER_FETCH - 1, [&chain, &error](BlockHeader & bh) {
                vector<Transaction> empty;
                Block block(blockHeader, empty);
                if (!block.verifyNonce()) error = true;
                if (block.getLastBlockHash() != lastHash) error = true;
                chain.lastHash = block.getHash();
                chain.totalWork+= block.getDifficulty();
                chain.numBlocks++;
                return error;
            });
            if (error) {
                // pop 100 blocks
                for(int j = 0; j < 100; j++) {
                    if (chain.size() == 0) break;
                    chain.popBlock();
                }
            }
            while(chain.size() > LOCAL_CHAIN_CACHE) {
                chain.discardBlock();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

HeaderChain::HeaderChain(string host, syncFrequency) {
    this->host = host;
    this->resetChain();
}

void HeaderChain::resetChain() {
    this->totalWork = 0;
    this->numBlocks = 0;
    this->lastHash = NULL_SHA256_HASH;
    this->headers = list<BlockHeader>();
}

void HeaderChain::setStartBlock(BlockHeader &b, uint64_t totalWork) {
    this->headers = list<BlockHeader>();
    this->headers.push_back(b);
    this->numBlocks = b.id;
    this->totalWork = totalWork;
    this->lastHash = b.getHash();
}

void HeaderChain::popBlock() {
    this->headers.pop_back();
    this->lastHash = this->headers.back().lastBlockHash;
}

void HeaderChain::discardBlock() {
    this->headers.pop_front();
    if (this->headers.size() == 0) {
        this->resetChain();
    }
}

uint64_t HeaderChain::getTotalWork();
    return this->totalWork;
};

uint64_t HeaderChain::getTotalWork();
    return this->numBlocks;
};