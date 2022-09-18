#include "../external/http.hpp"
#include <map>
#include <iostream>
#include <stdexcept>
#include <stdlib.h>
#include <map>
#include <algorithm>
#include <mutex>
#include <functional>
#include <thread>
#include <cstring>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <mutex>
#include "../core/merkle_tree.hpp"
#include "../core/logger.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/user.hpp"
#include "executor.hpp"
#include "virtual_chain.hpp"
#include "program.hpp"

using namespace std;

void chain_sync(VirtualChain& blockchain) {
    while(true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        if (blockchain.shutdown) break;
        blockchain.startChainSync();
    }
}

VirtualChain::VirtualChain(Program& program, HostManager& hosts) : program(program), hosts(hosts) {
    this->shutdown = false;
    this->initChain();
}

VirtualChain::~VirtualChain() {
    this->shutdown = true;
}

void VirtualChain::initChain() {
    this->isSyncing = false;
    if (program.hasBlockCount()) {
        size_t count = this->program.getBlockCount();
        this->targetBlockCount = count;
    } else {
        this->resetChain();
    }
}

void VirtualChain::resetChain() {
    this->targetBlockCount = 1;
    // reset the program ledger, block & tx stores
    this->program.clearState();
    Block genesis = program.getGenesis();
    ExecutionStatus status = this->addBlock(genesis);
    if (status != SUCCESS) {
        throw std::runtime_error("Could not add genesis block");
    }
}

ProgramID VirtualChain::getProgramForWallet(PublicWalletAddress addr) {
    throw std::runtime_error("Cannot load program for virtual chain");
}

std::pair<uint8_t*, size_t> VirtualChain::getRaw(uint32_t blockId) {
    if (blockId <= 0 || blockId > this->program.getBlockCount()) throw std::runtime_error("Invalid block");
    return this->program.getRawBlock(blockId);
}

BlockHeader VirtualChain::getBlockHeader(uint32_t blockId) {
    if (blockId <= 0 || blockId > this->program.getBlockCount()) throw std::runtime_error("Invalid block");
    return this->program.getBlockHeader(blockId);
}

void VirtualChain::sync() {
    this->syncThread.push_back(std::thread(chain_sync, ref(*this)));
}

uint32_t VirtualChain::getBlockCount() {
    return this->program.getBlockCount();
}

Bigint VirtualChain::getTotalWork() {
    return this->program.getTotalWork();
}

Block VirtualChain::getBlock(uint32_t blockId) {
    if (blockId <= 0 || blockId > this->program.getBlockCount()) throw std::runtime_error("Invalid block");
    return this->program.getBlock(blockId);
}

SHA256Hash VirtualChain::getLastHash() {
    return this->program.getLastHash();
}

TransactionAmount VirtualChain::getWalletValue(PublicWalletAddress addr) {
    return this->program.getWalletValue(addr);
}

void VirtualChain::updateDifficulty() {
    return;
}

TransactionAmount VirtualChain::getCurrentMiningFee() {
    return this->program.getCurrentMiningFee();
}

uint32_t VirtualChain::findBlockForTransactionId(SHA256Hash txid) {
    return this->program.blockForTransactionId(txid);
}

uint32_t VirtualChain::findBlockForTransaction(Transaction &t) {
    return this->program.blockForTransaction(t);
}

int VirtualChain::getDifficulty() {
    return this->program.getDifficulty();
}

vector<Transaction> VirtualChain::getTransactionsForWallet(const PublicWalletAddress& addr) const {
    vector<SHA256Hash> txids = this->program.getTransactionsForWallet(addr);
    vector<Transaction> ret;
    // TODO: this is pretty inefficient -- might want direct index of transactions
    for (auto txid : txids) {
        Block b = this->program.getBlock(this->program.blockForTransactionId(txid));
        for (auto tx : b.getTransactions()) {
            if (tx.hashContents() == txid) {
                ret.push_back(tx);
                break;
            }
        }
    }
    return std::move(ret);
}

void VirtualChain::popBlock() {
    Block last = this->getBlock(this->getBlockCount());
    this->program.rollbackBlock(last);
    if (this->getBlockCount() == 1) {
        this->resetChain();
    }
}

ExecutionStatus VirtualChain::addBlockSync(Block& block) {
    if (this->isSyncing) {
        return IS_SYNCING;
    } else {
        std::unique_lock<std::mutex> ul(lock);
        return this->addBlock(block);
    }
}
    
ExecutionStatus VirtualChain::addBlock(Block& block) {
    // check difficulty + nonce
    if (block.getTransactions().size() > MAX_TRANSACTIONS_PER_BLOCK) return INVALID_TRANSACTION_COUNT;
    if (block.getId() != this->program.getBlockCount() + 1) return INVALID_BLOCK_ID;
    if (block.getDifficulty() != this->program.getDifficulty()) return INVALID_DIFFICULTY;
    if (!block.verifyNonce()) return INVALID_NONCE;
    if (block.getLastBlockHash() != this->program.getLastHash()) return INVALID_LASTBLOCK_HASH;
    if (block.getId() != 1) {
        // block must be less than 2 hrs into future from network time
        uint64_t maxTime = this->hosts.getNetworkTimestamp() + 120*60;
        if (block.getTimestamp() > maxTime) return BLOCK_TIMESTAMP_IN_FUTURE;

        // block must be after the median timestamp of last 10 blocks
        if (this->program.getBlockCount() > 10) {
            vector<uint64_t> times;
            for(int i = 0; i < 10; i++) {
                Block b = this->getBlock(this->program.getBlockCount() - i);
                times.push_back(b.getTimestamp());
            }
            std::sort(times.begin(), times.end());
            // compute median
            uint64_t medianTime;
            if (times.size() % 2 == 0) {
                medianTime = (times[times.size()/2] + times[times.size()/2 - 1])/2;
            } else {
                medianTime = times[times.size()/2];
            }
            if (block.getTimestamp() < medianTime) return BLOCK_TIMESTAMP_TOO_OLD;
        }
    }
    // compute merkle tree and verify root matches;
    MerkleTree m;
    m.setItems(block.getTransactions());
    SHA256Hash computedRoot = m.getRootHash();
    if (block.getMerkleRoot() != computedRoot) return INVALID_MERKLE_ROOT;
    return this->program.executeBlock(block);
}


ExecutionStatus VirtualChain::startChainSync() {
    std::unique_lock<std::mutex> ul(lock);
    this->isSyncing = true;
    string bestHost = this->hosts.getGoodHost();
    this->targetBlockCount = this->hosts.getBlockCount();
    // If our current chain is lower POW than the trusted host
    // remove anything that does not align with the hashes of the trusted chain
    if (this->hosts.getTotalWork() > this->getTotalWork()) {
        // iterate through our current chain until a hash diverges from trusted chain
        uint64_t toPop = 0;
        for(uint64_t i = 1; i <= this->program.getBlockCount(); i++) {
            SHA256Hash trustedHash = this->hosts.getBlockHash(bestHost, i);
            SHA256Hash myHash = this->getBlock(i).getHash();
            if (trustedHash != myHash) {
                toPop = this->program.getBlockCount() - i + 1;
                break;
            }
        }
        // pop all subsequent blocks
        for (uint64_t i = 0; i < toPop; i++) {
            if (this->program.getBlockCount() == 1) break;
            this->popBlock();
        }
    }

    int startCount = this->program.getBlockCount();

    int needed = this->targetBlockCount - startCount;

    uint64_t start = std::time(0);
    for(int i = startCount + 1; i <= this->targetBlockCount; i+=BLOCKS_PER_FETCH) {
        try {
            int end = min(this->targetBlockCount, i + BLOCKS_PER_FETCH - 1);
            bool failure = false;
            ExecutionStatus status;
            VirtualChain &bc = *this;
            int count = 0;
            vector<Block> blocks;
            readRawBlocks(bestHost, i, end, blocks);
            for(auto & b : blocks) {   
                ExecutionStatus addResult = bc.addBlock(b);
                if (addResult != SUCCESS) {
                    failure = true;
                    status = addResult;
                    break;
                }
            }
            if (failure) {
                this->isSyncing = false;
                return status;
            }
        } catch (const std::exception &e) {
            this->isSyncing = false;
            return UNKNOWN_ERROR;
        }
    }
    this->isSyncing = false;
    return SUCCESS;
}


