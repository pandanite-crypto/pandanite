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
                std::pair<string, int> best = blockchain.hosts.getLongestChainHost();
                int toPop = 2 * (best.second - blockchain.getBlockCount());
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

BlockChain::BlockChain(HostManager& hosts) : hosts(hosts) {
    // check that genesis block file exists.
    ifstream f(GENESIS_FILE_PATH);
    if (!f.good()) {
        User miner;
        Block genesis;
        genesis.addTransaction(miner.mine(1));
        SHA256Hash nonce = mineHash(NULL_SHA256_HASH, genesis.getDifficulty());
        genesis.setNonce(nonce);
        writeJsonToFile(miner.toJson(), DEFAULT_GENESIS_USER_PATH);
        writeJsonToFile(genesis.toJson(), GENESIS_FILE_PATH);
    }
    this->resetChain();
}

void BlockChain::resetChain() {
    ledger.init(LEDGER_FILE_PATH);
    blockStore.init(BLOCK_STORE_FILE_PATH);
    for(size_t i = 0; i <this->lastHash.size(); i++) this->lastHash[i] = 0;
    json data = readJsonFromFile(GENESIS_FILE_PATH);
    Block genesis(data);
    this->difficulty = genesis.getDifficulty();
    this->targetBlockCount = 1;
    this->numBlocks = 0;
    ExecutionStatus status = this->addBlock(genesis);
    if (status != SUCCESS) {
        throw std::runtime_error("Could not load genesis block");
    }
}

std::pair<char*, size_t> BlockChain::getRaw(int blockId) {
    if (blockId < 0 || blockId > this->numBlocks) throw std::runtime_error("Invalid block");
    return this->blockStore.getRawData(blockId);
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

int BlockChain::getBlockCount() {
    return this->numBlocks;
}

Block BlockChain::getBlock(int blockId) {
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

void BlockChain::distributeTaxes(Block& block) {
    if (block.getId() >= TAX_PAYOUT_UNTIL) {
        ledger.setTaxRate(0);
    } else if (block.getId() % TAX_PAYOUT_FREQUENCY == 0) {
        ledger.payoutTaxes();
    }
}

void BlockChain::updateDifficulty(Block& block) {
    if(block.getId() % DIFFICULTY_RESET_FREQUENCY == 0) {
        // compute the new difficulty score based on average block time
        double average = 0;
        int total = 0;
        // ignore genesis block time
        int first = max(3, this->numBlocks - DIFFICULTY_RESET_FREQUENCY);
        int last = this->numBlocks - 1;
        for(int i = last; i >= first; i--) {
            Block b1 = this->blockStore.getBlock(i-1);
            Block b2 = this->blockStore.getBlock(i-2);
            int currTs = (int)b1.getTimestamp();
            int lastTs = (int)b2.getTimestamp();
            average += (double)(currTs - lastTs);
            total++;
        }
        average /= total;
        double ratio = average/DESIRED_BLOCK_TIME_SEC;
        int delta = -round(log2(ratio));
        this->difficulty += delta;
        this->difficulty = min(max(this->difficulty, MIN_DIFFICULTY), MAX_DIFFICULTY);
    }
}

int BlockChain::getDifficulty() {
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
    if (!block.verifyNonce(this->getLastHash())) return INVALID_NONCE;
    LedgerState deltasFromBlock;
    ExecutionStatus status = Executor::ExecuteBlock(block, this->ledger, deltasFromBlock);
    
    if (status != SUCCESS) {
        //revert ledger
        Executor::Rollback(this->ledger, deltasFromBlock);
    } else {
        this->blockStore.setBlock(block);
        this->numBlocks++;
        this->lastHash = block.getHash(concatHashes(this->getLastHash(), block.getNonce()));
        this->updateDifficulty(block);
        this->distributeTaxes(block);
    }
    return status;
}

ExecutionStatus BlockChain::startChainSync() {
    // iterate through each of the hosts and pick the longest one:
    std::pair<string,int> bestHostInfo = this->hosts.getLongestChainHost();
    string bestHost = bestHostInfo.first;
    this->targetBlockCount = bestHostInfo.second;
    int startCount = this->numBlocks + 1;

    if (startCount >= this->targetBlockCount) {
        return SUCCESS;
    }

    int needed = this->targetBlockCount - startCount;

    // download any remaining blocks in batches
    time_t start = std::time(0);
    for(int i = startCount; i <= this->targetBlockCount; i+=BLOCKS_PER_FETCH) {
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


