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

VirtualChain::VirtualChain(Program& program) {
    this->shutdown = false;
    this->program = program;
    this->hosts = hosts;
    this->initChain();
}

VirtualChain::~VirtualChain() {
    this->shutdown = true;
}

void VirtualChain::initChain() {
    this->isSyncing = false;
    if (program.hasBlockCount()) {
        size_t count = blockStore.getBlockCount();
        this->numBlocks = count;
        this->targetBlockCount = count;
        Block lastBlock = blockStore.getBlock(count);
        this->totalWork = blockStore.getTotalWork();
        this->difficulty = lastBlock.getDifficulty();
        this->lastHash = lastBlock.getHash();
    } else {
        this->resetChain();
    }
}

void VirtualChain::resetChain() {
    this->difficulty = 16;
    this->targetBlockCount = 1;
    this->numBlocks = 0;
    this->totalWork = 0;
    this->lastHash = NULL_SHA256_HASH;

    // reset the program ledger, block & tx stores
    this->program.clearState();

    ExecutionStatus status = this->addBlock(program.getGenesis());
}

ProgramID VirtualChain::getProgramForWallet(PublicWalletAddress addr) {
    throw std::runtime_error("Cannot load program for virtual chain");
}

std::pair<uint8_t*, size_t> VirtualChain::getRaw(uint32_t blockId) {
    if (blockId <= 0 || blockId > this->numBlocks) throw std::runtime_error("Invalid block");
    return this->program.getRawBlock(blockId);
}

BlockHeader VirtualChain::getBlockHeader(uint32_t blockId) {
    if (blockId <= 0 || blockId > this->numBlocks) throw std::runtime_error("Invalid block");
    return this->program.getBlockHeader(blockId);
}

void VirtualChain::sync() {
    this->syncThread.push_back(std::thread(chain_sync, ref(*this)));
}

uint32_t VirtualChain::getBlockCount() {
    return this->numBlocks;
}

Bigint VirtualChain::getTotalWork() {
    return this->totalWork;
}

Block VirtualChain::getBlock(uint32_t blockId) {
    if (blockId <= 0 || blockId > this->numBlocks) throw std::runtime_error("Invalid block");
    return this->program.getBlock(blockId);
}

ExecutionStatus VirtualChain::verifyTransaction(const Transaction& t) {
    if (this->isSyncing) return IS_SYNCING;

    LedgerState deltas;
    // verify the transaction is consistent with ledger
    std::unique_lock<std::mutex> ul(lock);
    ExecutionStatus status = this->program.executeTransaction(t, deltas);
    this->program.rollback(deltas);

    if (this->program.hasTransaction(t)) {
        status = EXPIRED_TRANSACTION;
    }

    return status;
}

SHA256Hash VirtualChain::getLastHash() {
    return this->lastHash;
}

TransactionAmount VirtualChain::getWalletValue(PublicWalletAddress addr) {
    return this->program.getWalletValue(addr);
}

uint32_t computeDifficulty(int32_t currentDifficulty, int32_t elapsedTime, int32_t expectedTime) {
    uint32_t newDifficulty = currentDifficulty;
    if (elapsedTime > expectedTime) {
        int k = 2;
        int lastK = 1;
        while(newDifficulty > MIN_DIFFICULTY) {
                if(abs(elapsedTime/k - expectedTime) > abs(elapsedTime/lastK - expectedTime) ) {
                    break;
                }
            newDifficulty--;
            lastK = k;
            k*=2;
        }
        return newDifficulty;
    } else {
        int k = 2;
        int lastK = 1;
        while(newDifficulty < 254) {
            if(abs(elapsedTime*k - expectedTime) > abs(elapsedTime*lastK - expectedTime) ) {
                break;
            }
            newDifficulty++;
            lastK = k;
            k*=2;
        }
        return newDifficulty;
    }
}

void VirtualChain::updateDifficulty() {
    if (this->numBlocks <= DIFFICULTY_LOOKBACK*2) return;
    if (this->numBlocks % DIFFICULTY_LOOKBACK != 0) return;
    int firstID = this->numBlocks - DIFFICULTY_LOOKBACK;
    int lastID = this->numBlocks;  
    Block first = this->getBlock(firstID);
    Block last = this->getBlock(lastID);
    int32_t elapsed = last.getTimestamp() - first.getTimestamp(); 
    uint32_t numBlocksElapsed = lastID - firstID;
    int32_t target = numBlocksElapsed * DESIRED_BLOCK_TIME_SEC;
    int32_t difficulty = last.getDifficulty();
    this->difficulty = computeDifficulty(difficulty, elapsed, target);
}

uint32_t VirtualChain::findBlockForTransactionId(SHA256Hash txid) {
    uint32_t blockId = this->program.blockForTransactionId(txid);
    return blockId;
}

uint32_t VirtualChain::findBlockForTransaction(Transaction &t) {
    uint32_t blockId = this->program.blockForTransaction(t);
    return blockId;
}

uint8_t VirtualChain::getDifficulty() {
    return this->difficulty;
}

vector<Transaction> VirtualChain::getTransactionsForWallet(PublicWalletAddress addr) {
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
    this->numBlocks--;
    Bigint base = 2;
    this->totalWork -= base.pow((int)last.getDifficulty());
    if (this->getBlockCount() > 1) {
        Block newLast = this->getBlock(this->getBlockCount());
        this->difficulty = newLast.getDifficulty();
        this->lastHash = newLast.getHash();
    } else {
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
    if (block.getId() != this->numBlocks + 1) return INVALID_BLOCK_ID;
    if (block.getDifficulty() != this->difficulty) return INVALID_DIFFICULTY;
    if (!block.verifyNonce()) return INVALID_NONCE;
    if (block.getLastBlockHash() != this->getLastHash()) return INVALID_LASTBLOCK_HASH;
    if (block.getId() != 1) {
        // block must be less than 2 hrs into future from network time
        uint64_t maxTime = this->hosts.getNetworkTimestamp() + 120*60;
        if (block.getTimestamp() > maxTime) return BLOCK_TIMESTAMP_IN_FUTURE;

        // block must be after the median timestamp of last 10 blocks
        if (this->numBlocks > 10) {
            vector<uint64_t> times;
            for(int i = 0; i < 10; i++) {
                Block b = this->getBlock(this->numBlocks - i);
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
    LedgerState deltasFromBlock;
    ExecutionStatus status = this->program.executeBlock(block, deltasFromBlock);

    if (status != SUCCESS) {
        this->program.rollback(deltasFromBlock);
    } else {
        // add all transactions to txdb:
        for(auto t : block.getTransactions()) {
            this->program.insertTransaction(t, block.getId());
        }
        this->program.setBlock(block);
        this->numBlocks++;
        this->totalWork = addWork(this->totalWork, block.getDifficulty());
        this->program.setTotalWork(this->totalWork);
        this->program.setBlockCount(this->numBlocks);
        this->lastHash = block.getHash();
        this->updateDifficulty();
    }
    return status;
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
        for(uint64_t i = 1; i <= this->numBlocks; i++) {
            SHA256Hash trustedHash = this->hosts.getBlockHash(bestHost, i);
            SHA256Hash myHash = this->getBlock(i).getHash();
            if (trustedHash != myHash) {
                toPop = this->numBlocks - i + 1;
                break;
            }
        }
        // pop all subsequent blocks
        for (uint64_t i = 0; i < toPop; i++) {
            if (this->numBlocks == 1) break;
            this->popBlock();
        }
    }

    int startCount = this->numBlocks;

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


