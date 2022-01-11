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
#include "genesis.hpp"

#define FORK_CHAIN_POP_COUNT 15
#define FORK_RESET_RETRIES 3
#define MAX_DISCONNECTS_BEFORE_RESET 10
#define FAILURES_BEFORE_POP_ATTEMPT 1

using namespace std;

void chain_sync(BlockChain& blockchain) {
    unsigned long i = 0;
    int failureCount = 0;
    int connectionFailureCount = 0;
    int chainPopCount = 0;
    while(true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        blockchain.acquire();
        try {
            ExecutionStatus valid;
            valid = blockchain.startChainSync();
            connectionFailureCount = 0;
            if (valid != SUCCESS) {
                Logger::logStatus("chain_sync: peer chain diverges: " + executionStatusAsString(valid));
                failureCount++;
            } else {
                failureCount = 0;
            }

            if (chainPopCount > FORK_RESET_RETRIES) {
                blockchain.hosts.initTrustedHost();
                chainPopCount = 0;
                failureCount = 0;
            }
            if (failureCount > FAILURES_BEFORE_POP_ATTEMPT) {
                int toPop = FORK_CHAIN_POP_COUNT;
                if (blockchain.getBlockCount() < FORK_CHAIN_POP_COUNT + 1) {
                    blockchain.resetChain();
                } else {
                    Logger::logStatus("chain_sync: removing " + to_string(toPop) + " blocks and re-trying.");
                    for(int j = 0; j < toPop; j++) {
                        blockchain.popBlock();
                    }
                    chainPopCount += 1;
                }
                failureCount = 0;
            }
        } catch(std::exception & e) {
            try {
                connectionFailureCount++;
                if (connectionFailureCount > MAX_DISCONNECTS_BEFORE_RESET) {
                    Logger::logError("chain_sync", string(e.what()));
                    blockchain.hosts.initTrustedHost();
                    connectionFailureCount = 0;
                }
            } catch (...) {
                Logger::logError("chain_sync", "No new host found. Running as solo node.");
            }
        }
        blockchain.release();
        i++;
        // resync headers and find new peer ~27 hrs
        if (i % 10000 == 0) blockchain.hosts.initTrustedHost();
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

    // reset the ledger, block & tx stores
    this->ledger.clear();
    this->blockStore.clear();
    this->txdb.clear();
    

    // used to create genesis.json
    // Block genesis;
    // User miner;
    // Transaction fee(stringToWalletAddress("0095557B94A368FE2529D3EB33E6BF1276D175D27A4E876249"), BMB(50));
    // fee.setTimestamp(0);
    // vector<Transaction> transactions;
    
    // genesis.setTimestamp(0);
    // genesis.setId(1);
    // genesis.addTransaction(fee);
    // genesis.setLastBlockHash(NULL_SHA256_HASH);
    // addGenesisTransactions(genesis);
    
    // // compute merkle tree
    // MerkleTree m;
    // m.setItems(genesis.getTransactions());
    // SHA256Hash computedRoot = m.getRootHash();
    // genesis.setMerkleRoot(m.getRootHash());

    // SHA256Hash hash = genesis.getHash();

    // SHA256Hash solution = mineHash(hash, genesis.getDifficulty());
    // genesis.setNonce(solution);

    // writeJsonToFile(genesis.toJson(), "genesis.json");
    
    json genesisData = readJsonFromFile("genesis.json");
    Block genesis(genesisData);

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

Bigint BlockChain::getTotalWork() {
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

    if (this->txdb.hasTransaction(t)) {
        status = EXPIRED_TRANSACTION;
    }

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
    Bigint base = 2;
    this->totalWork -= base.pow((int)last.getDifficulty());
    this->blockStore.setTotalWork(this->totalWork);
    this->blockStore.setBlockCount(this->numBlocks);
    if (this->getBlockCount() > 1) {
        Block newLast = this->getBlock(this->getBlockCount());
        this->difficulty = newLast.getDifficulty();
        this->lastHash = newLast.getHash();
    } else {
        this->resetChain();
    }
    
}

ExecutionStatus BlockChain::addBlock(Block& block) {
    // check difficulty + nonce
    if (block.getTransactions().size() > MAX_TRANSACTIONS_PER_BLOCK) return INVALID_TRANSACTION_COUNT;
    if (block.getId() != this->numBlocks + 1) return INVALID_BLOCK_ID;
    if (block.getDifficulty() != this->difficulty) {
        Logger::logStatus("Mine: " + to_string(this->difficulty) + " THEIRS: " + to_string(block.getDifficulty()));
        return INVALID_DIFFICULTY;
    }
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
    ExecutionStatus status = Executor::ExecuteBlock(block, this->ledger, this->txdb, deltasFromBlock, this->getCurrentMiningFee());

    if (status != SUCCESS) {
        Executor::Rollback(this->ledger, deltasFromBlock);
    } else {
        if (this->memPool != nullptr) {
            this->memPool->finishBlock(block);
        }
        // add all transactions to txdb:
        for(auto t : block.getTransactions()) {
            if (!t.isFee()) this->txdb.insertTransaction(t, block.getId());
        }
        this->blockStore.setBlock(block);
        this->numBlocks++;
        Bigint base = 2;
        this->totalWork += base.pow((int)block.getDifficulty());
        this->blockStore.setTotalWork(this->totalWork);
        this->blockStore.setBlockCount(this->numBlocks);
        this->lastHash = block.getHash();
        this->updateDifficulty();
        Logger::logStatus("Added block " + to_string(block.getId()));
        Logger::logStatus("difficulty= " + to_string(block.getDifficulty()));
    }
    return status;
}

ExecutionStatus BlockChain::startChainSync() {
    // get chain w/ highest POW amongst random peers
    std::pair<string,uint64_t> bestHostInfo = this->hosts.getTrustedHost();

    uint64_t checkHashesUntilBlock = bestHostInfo.second; // NOTE: bestHostInfo.second = length of trusted chain

    // If our current chain is lower POW than the trusted host
    // remove anything that does not align with the hashes of the trusted chain
    if (this->hosts.getTrustedHostWork() > this->getTotalWork()) {
        // iterate through our current chain until a hash diverges from trusted chain
        Logger::logStatus("Finding chain divergance");
        uint64_t toPop = 0;
        for(uint64_t i = 1; i <= this->numBlocks; i++) {
            SHA256Hash trustedHash = this->hosts.getBlockHash(i);
            SHA256Hash myHash = this->getBlock(i).getHash();
            if (trustedHash != myHash) {
                toPop = this->numBlocks - i + 1;
                Logger::logStatus("toPop = " + to_string(toPop));
                break;
            }
        }
        // pop all subsequent blocks
        for (uint64_t i = 0; i < toPop; i++) {
            if (this->numBlocks == 1) break;
            this->popBlock();
        }
    }

    string bestHost = bestHostInfo.first;
    this->targetBlockCount = getCurrentBlockCount(bestHost);

    int startCount = this->numBlocks;

    int needed = this->targetBlockCount - startCount;

    // download any remaining blocks in batches
    uint64_t start = std::time(0);
    for(int i = startCount + 1; i <= this->targetBlockCount; i+=BLOCKS_PER_FETCH) {
        try {
            int end = min(this->targetBlockCount, i + BLOCKS_PER_FETCH - 1);
            bool failure = false;
            ExecutionStatus status;
            BlockChain &bc = *this;
            int count = 0;
            readRawBlocks(bestHost, i, end, [&](Block& b) {
                if (!failure) {
                    //check that the host sent same block as trusted chain headers:
                    if (b.getId() < checkHashesUntilBlock && b.getHash() != bc.hosts.getBlockHash(b.getId())) {                        
                        status = INVALID_LASTBLOCK_HASH;
                        failure = true;
                        Logger::logError("Header Chain does not match block. Chain failed at blockID", std::to_string(b.getId()));
                    } else {
                        ExecutionStatus addResult = bc.addBlock(b);
                        if (addResult != SUCCESS) {
                            failure = true;
                            status = addResult;
                            Logger::logError("Chain failed at blockID", std::to_string(b.getId()));
                        }
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
    s<<"Downloaded " << needed <<" blocks in " << d << " seconds from " + bestHost;
    if (needed > 1) Logger::logStatus(s.str());
    return SUCCESS;
}


