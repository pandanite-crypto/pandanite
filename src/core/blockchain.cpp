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
#include "merkle_tree.hpp"
#include "logger.hpp"
#include "blockchain.hpp"
#include "helpers.hpp"
#include "user.hpp"
#include "api.hpp"

using namespace std;

void chain_sync(BlockChain& blockchain) {
    unsigned long i = 0;
    int failureCount = 0;
    while(true) {
        blockchain.acquire();
        try {
            ExecutionStatus valid;
            valid = blockchain.startChainSync();
            if (valid != SUCCESS) {
                failureCount++;
            }
            if (failureCount > 3) {
                std::pair<string, int> best = blockchain.hosts.getBestHost();
                int toPop = min(2*(best.second - blockchain.getBlockCount()), blockchain.getBlockCount() - 1);
                Logger::logStatus("chain_sync: out of sync, removing " + to_string(toPop) + " blocks and re-trying.");
                for(int j = 0; j < toPop; j++) {
                    blockchain.popBlock();
                }
                failureCount = 0;
            }
        } catch(std::exception & e) {
            Logger::logError("chain_sync", string(e.what()));
        }
        blockchain.release();
        if (i%2000 == 0) { // refresh host list roughly every 3 min
            Logger::logStatus("chain_sync: Refreshing host list");
            blockchain.hosts.refreshHostList();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        i++;
    }
}

BlockChain::BlockChain(HostManager& hosts, string ledgerPath, string blockPath) : hosts(hosts) {
    if (ledgerPath == "") ledgerPath = LEDGER_FILE_PATH;
    if (blockPath == "") blockPath = BLOCK_STORE_FILE_PATH;
    ledger.init(ledgerPath);
    blockStore.init(blockPath);
    this->resetChain();
}

void BlockChain::resetChain() {
    if (blockStore.hasBlockCount()) {
        Logger::logStatus("BlockStore exists, loading from disk");
        size_t count = blockStore.getBlockCount();
        this->numBlocks = count;
        this->targetBlockCount = count;
        Block lastBlock = blockStore.getBlock(count);
        this->totalWork = blockStore.getTotalWork();
        this->difficulty = lastBlock.getDifficulty();
        this->lastHash = lastBlock.getHash();
    } else {
        Logger::logStatus("BlockStore does not exist");
        for(size_t i = 0; i <this->lastHash.size(); i++) this->lastHash[i] = 0;
        json data = readJsonFromFile(GENESIS_FILE_PATH);
        Block genesis(data);
        this->difficulty = genesis.getDifficulty();
        this->targetBlockCount = 1;
        this->numBlocks = 0;
        this->totalWork = 0;
        this->lastHash = NULL_SHA256_HASH;
        ExecutionStatus status = this->addBlock(genesis);
        if (status != SUCCESS) {
            throw std::runtime_error("Could not load genesis block: " + executionStatusAsString(status));
        }
    }
}

void BlockChain::deleteDB() {
    ledger.closeDB();
    blockStore.closeDB();
    ledger.deleteDB();
    blockStore.deleteDB();
}

std::pair<uint8_t*, size_t> BlockChain::getRaw(uint32_t blockId) {
    if (blockId < 0 || blockId > this->numBlocks) throw std::runtime_error("Invalid block");
    return this->blockStore.getRawData(blockId);
}

std::pair<uint8_t*, size_t> BlockChain::getBlockHeaders() {
    return this->blockStore.getBlockHeaders();
}

void BlockChain::sync() {
    this->syncThread.push_back(std::thread(chain_sync, ref(*this)));
}

void BlockChain::acquire() {
    this->lock.lock();
}
void BlockChain::release() {
    this->lock.unlock();
}

Ledger& BlockChain::getLedger() {
    return this->ledger;
}

uint32_t BlockChain::getBlockCount() {
    return this->numBlocks;
}

uint64_t BlockChain::getTotalWork() {
    return this->totalWork;
}

Block BlockChain::getBlock(uint32_t blockId) {
    if (blockId < 0 || blockId > this->numBlocks) throw std::runtime_error("Invalid block");
    return this->blockStore.getBlock(blockId);
}

ExecutionStatus BlockChain::verifyTransaction(const Transaction& t) {
    if (!t.signatureValid()) return INVALID_SIGNATURE;
    LedgerState deltas;
    // verify the transaction is consistent with ledger
    ExecutionStatus status = Executor::ExecuteTransaction(this->getLedger(), t, deltas);

    //roll back the ledger to it's original state:
    Executor::Rollback(this->getLedger(), deltas);
    return status;
}

SHA256Hash BlockChain::getLastHash() {
    return this->lastHash;
}

void BlockChain::updateDifficulty(Block& block) {
    if(block.getId() % DIFFICULTY_RESET_FREQUENCY == 0) {
        // compute the new difficulty score based on average block time
        long average = 0;
        uint32_t total = 0;
        // ignore genesis block time
        uint32_t first = max(3, (int)this->numBlocks - DIFFICULTY_RESET_FREQUENCY);
        uint32_t last = this->numBlocks - 1;
        for(uint32_t i = last; i >= first; i--) {
            Block b1 = this->blockStore.getBlock(i-1);
            Block b2 = this->blockStore.getBlock(i-2);
            uint32_t currTs = (uint32_t)b1.getTimestamp();
            uint32_t lastTs = (uint32_t)b2.getTimestamp();
            average += (double)(currTs - lastTs);
            total++;
        }
        long scaleFactor = 1000000000;
        average *= scaleFactor;
        average /= total;
        long ratio = average/(DESIRED_BLOCK_TIME_SEC * scaleFactor);
        int delta = -round(log2(ratio));
        this->difficulty += delta;
        this->difficulty = min(max((int)this->difficulty, MIN_DIFFICULTY), MAX_DIFFICULTY);
    }
}

uint8_t BlockChain::getDifficulty() {
    return this->difficulty;
}

void BlockChain::popBlock() {
    Block last = this->getBlock(this->getBlockCount());
    Executor::RollbackBlock(last, this->ledger);
    this->numBlocks--;
}

ExecutionStatus BlockChain::addBlock(Block& block) {
    // check difficulty + nonce
    if (block.getId() != this->numBlocks + 1) return INVALID_BLOCK_ID;
    if (block.getDifficulty() != this->difficulty) return INVALID_DIFFICULTY;
    if (!block.verifyNonce()) return INVALID_NONCE;
    if (block.getLastBlockHash() != this->getLastHash()) return INVALID_LASTBLOCK_HASH;
    // compute merkle tree and verify root matches;
    MerkleTree m;
    m.setItems(block.getTransactions());
    SHA256Hash computedRoot = m.getRootHash();
    if (block.getMerkleRoot() != computedRoot) return INVALID_MERKLE_ROOT;
    LedgerState deltasFromBlock;
    ExecutionStatus status = Executor::ExecuteBlock(block, this->ledger, deltasFromBlock);
    if (status != SUCCESS) {
        //revert ledger
        Executor::Rollback(this->ledger, deltasFromBlock);
    } else {
        this->blockStore.setBlock(block);
        this->numBlocks++;
        this->totalWork += block.getDifficulty();
        this->blockStore.setTotalWork(this->totalWork);
        this->blockStore.setBlockCount(this->numBlocks);
        this->lastHash = block.getHash();
        this->updateDifficulty(block);
    }
    return status;
}

ExecutionStatus BlockChain::startChainSync() {
    // iterate through each of the hosts and pick the longest one:
    std::pair<string,int> bestHostInfo = this->hosts.getBestHost();
    string bestHost = bestHostInfo.first;
    this->targetBlockCount = bestHostInfo.second;
    int startCount = this->numBlocks;

    if (startCount >= this->targetBlockCount) {
        return SUCCESS;
    }

    int needed = this->targetBlockCount - startCount;

    // download any remaining blocks in batches
    time_t start = std::time(0);
    for(int i = startCount + 1; i <= this->targetBlockCount; i+=BLOCKS_PER_FETCH) {
        try {
            int end = min(this->targetBlockCount, i + BLOCKS_PER_FETCH - 1);
            bool failure = false;
            ExecutionStatus status;
            BlockChain &bc = *this;
            int count = 0;
            readRaw(bestHost, i, end, [&bc, &failure, &status, &count](Block& b) {
                if (!failure) {
                    ExecutionStatus addResult = bc.addBlock(b);
                    if (addResult != SUCCESS) {
                        failure = true;
                        status = addResult;
                        Logger::logError("Chain failed at blockID", std::to_string(b.getId()));
                    }
                } 
            });
            if (failure) {
                Logger::logError("BlockChain::startChainSync", executionStatusAsString(status));
                return status;
            }
        } catch (const std::exception &e) {
            Logger::logError("BlockChain::startChainSync", "Failed to load block" + string(e.what()));
            return UNKNOWN_ERROR;
        }
    }
    time_t final = std::time(0);
    time_t d = final - start;
    stringstream s;
    s<<"Downloaded " << needed <<" blocks in " << d << " seconds";
    if (needed > 1) Logger::logStatus(s.str());
    return SUCCESS;
}


