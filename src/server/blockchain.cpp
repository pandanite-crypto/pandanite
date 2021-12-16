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
#include "../core/merkle_tree.hpp"
#include "../core/logger.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "blockchain.hpp"


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
                Logger::logStatus("chain_sync: failed: " + executionStatusAsString(valid));
                failureCount++;
            }
            if (failureCount > 3) {
                int toPop = 50;
                if (blockchain.getBlockCount() - toPop <=10) {
                    Logger::logStatus("chain_sync: 3 failures,resetting chain.");
                    blockchain.resetChain();
                } else {
                    Logger::logStatus("chain_sync: 3 failures, removing " + to_string(toPop) + " blocks and re-trying.");
                    for(int j = 0; j < toPop; j++) {
                        blockchain.popBlock();
                    }
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

BlockChain::BlockChain(HostManager& hosts, string ledgerPath, string blockPath, string txdbPath) : hosts(hosts) {
    if (ledgerPath == "") ledgerPath = LEDGER_FILE_PATH;
    if (blockPath == "") blockPath = BLOCK_STORE_FILE_PATH;
    if (txdbPath == "") txdbPath = TXDB_FILE_PATH;
    ledger.init(ledgerPath);
    blockStore.init(blockPath);
    txdb.init(txdbPath);
    this->initChain();
}

void BlockChain::initChain() {
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
        this->resetChain();
    }
}

void BlockChain::resetChain() {
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

void BlockChain::closeDB() {
    txdb.closeDB();
    ledger.closeDB();
    blockStore.closeDB();
}

void BlockChain::deleteDB() {
    this->closeDB();
    txdb.deleteDB();
    ledger.deleteDB();
    blockStore.deleteDB();
}

std::pair<uint8_t*, size_t> BlockChain::getRaw(uint32_t blockId) {
    if (blockId < 0 || blockId > this->numBlocks) throw std::runtime_error("Invalid block");
    return this->blockStore.getRawData(blockId);
}

std::pair<uint8_t*, size_t> BlockChain::getBlockHeaders(uint32_t start, uint32_t end) {
    return this->blockStore.getBlockHeaders(start, end);
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
    if (blockId <= 0 || blockId > this->numBlocks) throw std::runtime_error("Invalid block");
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

int computeDifficulty(int32_t currentDifficulty, int32_t elapsedTime, int32_t expectedTime) {
    uint32_t newDifficulty = currentDifficulty;
    if (elapsedTime > expectedTime) {
            int k = 2;
            int lastK = 1;
            while(newDifficulty > 16) {
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
        while(newDifficulty < 255) {
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



#define BLOCK_DIFFICULTY_GAP_START 37800
#define BLOCK_DIFFICULTY_GAP_END   40000

void BlockChain::updateDifficulty(Block& block) {
    if(block.getId() % DIFFICULTY_RESET_FREQUENCY == 0) {
        if (block.getId() >= BLOCK_DIFFICULTY_GAP_END) {
            int firstID = max(3, this->numBlocks - DIFFICULTY_RESET_FREQUENCY); 
            int lastID = this->numBlocks;  
            Block first = this->getBlock(firstID);
            Block last = this->getBlock(lastID);
            int32_t elapsed = last.getTimestamp() - first.getTimestamp(); 
            uint32_t numBlocksElapsed = lastID - firstID;
            int32_t target = numBlocksElapsed * DESIRED_BLOCK_TIME_SEC;
            int32_t difficulty = last.getDifficulty();
            this->difficulty = computeDifficulty(difficulty, elapsed, target);
        } else if (block.getId() > NEW_DIFFICULTY_COMPUTATION_BLOCK) {
            int firstID = max(3, this->numBlocks - DIFFICULTY_RESET_FREQUENCY);
            int lastID = this->numBlocks - 1;
            Block first = this->getBlock(firstID);
            Block last = this->getBlock(lastID);
            int32_t elapsed = last.getTimestamp() - first.getTimestamp();
            int32_t target = DIFFICULTY_RESET_FREQUENCY * DESIRED_BLOCK_TIME_SEC;
            int32_t difficulty = last.getDifficulty();
            this->difficulty = computeDifficulty(difficulty, elapsed, target);
        } else {
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
}

uint8_t BlockChain::getDifficulty() {
    return this->difficulty;
}

void BlockChain::popBlock() {
    Block last = this->getBlock(this->getBlockCount());
    Executor::RollbackBlock(last, this->ledger, this->txdb);
    this->numBlocks--;
    this->totalWork -= last.getDifficulty();
    this->blockStore.setTotalWork(this->totalWork);
    this->blockStore.setBlockCount(this->numBlocks);
    if (this->getBlockCount() > 1) {
        Block newLast = this->getBlock(this->getBlockCount());
        this->updateDifficulty(newLast);
        this->lastHash = newLast.getHash();
    } else {
        this->resetChain();
    }
    
}

ExecutionStatus BlockChain::addBlock(Block& block) {
    // check difficulty + nonce
    if (block.getId() != this->numBlocks + 1) return INVALID_BLOCK_ID;
    if (block.getDifficulty() != this->difficulty && !(block.getId() >= BLOCK_DIFFICULTY_GAP_START && block.getId() < BLOCK_DIFFICULTY_GAP_END)) {
        Logger::logStatus("Added block difficulty: " + std::to_string(block.getDifficulty()) + " chain difficulty: " + to_string(this->difficulty));
        return INVALID_DIFFICULTY;
    }
    if (!block.verifyNonce()) return INVALID_NONCE;
    if (block.getLastBlockHash() != this->getLastHash()) return INVALID_LASTBLOCK_HASH;
    // if we are greater than block 20700 check timestamps
    if (block.getId() > TIMESTAMP_VERIFICATION_START) {
        Block lastBlock = blockStore.getBlock(this->getBlockCount());
        if (block.getTimestamp() < lastBlock.getTimestamp()) return BLOCK_TIMESTAMP_TOO_OLD;
    }

    // compute merkle tree and verify root matches;
    MerkleTree m;
    m.setItems(block.getTransactions());
    SHA256Hash computedRoot = m.getRootHash();
    if (block.getMerkleRoot() != computedRoot) return INVALID_MERKLE_ROOT;
    LedgerState deltasFromBlock;
    ExecutionStatus status = Executor::ExecuteBlock(block, this->ledger, this->txdb, deltasFromBlock);
    if (status != SUCCESS) {
        //revert ledger
        cout<<"STATUS: " << executionStatusAsString(status)<<endl;
        Executor::RollbackBlock(block, this->ledger, this->txdb);
    } else {
        Logger::logStatus("Added block " + to_string(block.getId()));
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


