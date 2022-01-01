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
#include "../core/user.hpp"
#include "blockchain.hpp"
#include "mempool.hpp"

using namespace std;

#define MAX_VALIDATOR_DISCREPANCY 3

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
    this->memPool = nullptr;
    this->ledger.init(ledgerPath);
    this->blockStore.init(blockPath);
    this->txdb.init(txdbPath);
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
    this->difficulty = MIN_DIFFICULTY;
    this->targetBlockCount = 1;
    this->numBlocks = 0;
    this->totalWork = 0;
    this->lastHash = NULL_SHA256_HASH;

    /** TODO: add totals for existing miners **/
    User miner;
    Transaction fee = miner.mine();
    vector<Transaction> transactions;
    Block genesis;
    genesis.setId(1);
    genesis.addTransaction(fee);
    genesis.setLastBlockHash(NULL_SHA256_HASH);
    SHA256Hash hash = genesis.getHash();
    
    // compute merkle tree
    MerkleTree m;
    m.setItems(genesis.getTransactions());
    SHA256Hash computedRoot = m.getRootHash();
    genesis.setMerkleRoot(m.getRootHash());

    SHA256Hash solution = mineHash(hash, genesis.getDifficulty());
    genesis.setNonce(solution);

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
    if (blockId <= 0 || blockId > this->numBlocks) throw std::runtime_error("Invalid block");
    return this->blockStore.getRawData(blockId);
}

BlockHeader BlockChain::getBlockHeader(uint32_t blockId) {
    if (blockId <= 0 || blockId > this->numBlocks) throw std::runtime_error("Invalid block");
    return this->blockStore.getBlockHeader(blockId);
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

uint32_t BlockChain::getCurrentMiningFee() {
    if (this->numBlocks < 1000000) {
        return BMB(50.0);
    } else if (this->numBlocks < 2000000) {
        return BMB(25.0);
    }  else if (this->numBlocks < 4000000) {
        return BMB(12.5);
    } else {
        return BMB(0.0);
    }
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
    this->lock.lock();
    ExecutionStatus status = Executor::ExecuteTransaction(this->getLedger(), t, deltas);

    //roll back the ledger to it's original state:
    Executor::Rollback(this->getLedger(), deltas);
    this->lock.unlock();
    return status;
}

SHA256Hash BlockChain::getLastHash() {
    return this->lastHash;
}

TransactionAmount BlockChain::getWalletValue(PublicWalletAddress addr) {
    return this->getLedger().getWalletValue(addr);
}

uint32_t computeDifficulty(int32_t currentDifficulty, int32_t elapsedTime, int32_t expectedTime) {
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

void BlockChain::updateDifficulty() {
    if (this->numBlocks <= DIFFICULTY_LOOKBACK) return;
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

uint32_t BlockChain::findBlockForTransaction(Transaction &t) {
    uint32_t blockId = this->txdb.blockForTransaction(t);
    return blockId;
}

void BlockChain::setMemPool(MemPool * memPool) {
    this->memPool = memPool;
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
        this->updateDifficulty();
        this->lastHash = newLast.getHash();
    } else {
        this->resetChain();
    }
    
}

ExecutionStatus BlockChain::addBlock(Block& block) {
    // check difficulty + nonce
    if (block.getId() != this->numBlocks + 1) return INVALID_BLOCK_ID;
    if (block.getDifficulty() != this->difficulty) return INVALID_DIFFICULTY;
    if (!block.verifyNonce()) return INVALID_NONCE;
    if (block.getLastBlockHash() != this->getLastHash()) return INVALID_LASTBLOCK_HASH;
    if (block.getId() != 1) {
        Block lastBlock = blockStore.getBlock(this->getBlockCount());
        if (block.getTimestamp() < lastBlock.getTimestamp()) return BLOCK_TIMESTAMP_TOO_OLD;
    }
    // compute merkle tree and verify root matches;
    MerkleTree m;
    m.setItems(block.getTransactions());
    SHA256Hash computedRoot = m.getRootHash();
    if (block.getMerkleRoot() != computedRoot) return INVALID_MERKLE_ROOT;
    LedgerState deltasFromBlock;
    ExecutionStatus status = Executor::ExecuteBlock(block, this->ledger, this->txdb, deltasFromBlock, this->getCurrentMiningFee());
    if (status != SUCCESS) {
        //revert ledger
        Executor::RollbackBlock(block, this->ledger, this->txdb);
    } else {
        if (this->memPool != nullptr) {
            this->memPool->finishBlock(block);
        }
        Logger::logStatus("Added block " + to_string(block.getId()));
        this->blockStore.setBlock(block);
        this->numBlocks++;
        this->totalWork += block.getDifficulty();
        this->blockStore.setTotalWork(this->totalWork);
        this->blockStore.setBlockCount(this->numBlocks);
        this->lastHash = block.getHash();
        this->updateDifficulty();
    }
    return status;
}

ExecutionStatus BlockChain::startChainSync() {
    // pick a random host to download block data from:
    std::pair<string,int> bestHostInfo = this->hosts.getRandomHost();
    string bestHost = bestHostInfo.first;

    // pick a random host to validate the blocks against:
    std::pair<string,int> validatorHostInfo = this->hosts.getRandomHost();
    string validationHost = validatorHostInfo.first;

    // make sure they don't diverge by too many blocks
    if (abs(validatorHostInfo.second - bestHostInfo.second) > MAX_VALIDATOR_DISCREPANCY) return HOSTS_DIVERGE;

    // only download a # of blocks across consistent to both hosts:
    this->targetBlockCount = min(validatorHostInfo.second, bestHostInfo.second);
    int startCount = this->numBlocks;

    if (startCount >= this->targetBlockCount) {
        return SUCCESS;
    }

    int needed = this->targetBlockCount - startCount;


    // download any remaining blocks in batches
    uint64_t start = std::time(0);
    for(int i = startCount + 1; i <= this->targetBlockCount; i+=BLOCKS_PER_FETCH) {
        try {
            int end = min(this->targetBlockCount, i + BLOCKS_PER_FETCH - 1);


            // read block headers from validator node 
            vector<BlockHeader> validationHeaders;
            readRawHeaders(validationHost, i, end, [&validationHeaders](BlockHeader& b) {
                validationHeaders.push_back(b);
            });

            SHA256Hash vLastHash = this->getLastHash();

            // check that the validator nodes headers form a valid POW chain (otherwise fail)
            vector<SHA256Hash> hashCheck;
            for(auto header : validationHeaders) {
                vector<Transaction> empty;
                Block block(header, empty);
                if (!block.verifyNonce()) {
                    return INVALID_NONCE;
                } 

                if (block.getLastBlockHash() != vLastHash) {
                    return INVALID_LASTBLOCK_HASH;
                }
                vLastHash = block.getHash();
                hashCheck.push_back(vLastHash);
            }

            bool failure = false;
            ExecutionStatus status;
            BlockChain &bc = *this;
            int count = 0;
            readRawBlocks(bestHost, i, end, [&bc, &failure, &status, &count, &hashCheck](Block& b) {
                if (!failure) {
                    ExecutionStatus addResult = bc.addBlock(b);
                    if (hashCheck[count] != b.getHash()) {
                        failure = true;
                        status = INVALID_LASTBLOCK_HASH;
                    } else if (addResult != SUCCESS) {
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
    uint64_t final = std::time(0);
    uint64_t d = final - start;
    stringstream s;
    s<<"Downloaded " << needed <<" blocks in " << d << " seconds";
    if (needed > 1) Logger::logStatus(s.str());
    return SUCCESS;
}


