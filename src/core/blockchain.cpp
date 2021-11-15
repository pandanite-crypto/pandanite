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
#include "blockchain.hpp"
#include "helpers.hpp"
#include "user.hpp"
#include "api.hpp"

using namespace std;

void chain_sync(BlockChain& blockchain) {
    int popOnFailure = 1;
    while(true) {
        ExecutionStatus valid;
        blockchain.acquire();
        valid = blockchain.startChainSync();
        if (valid==SUCCESS) {
            cout<<"chain in sync, block count: "<<blockchain.getBlockCount()<<endl;
        }
        blockchain.release();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

BlockChain::BlockChain() {
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
        cout<<"Could not load genesis block"<<endl;
        cout<<executionStatusAsString(status)<<endl;
    }
}

void BlockChain::sync(vector<string> hosts) {
    this->hosts = hosts;
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

ExecutionStatus BlockChain::verifyTransaction(Transaction& t) {
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
        if (this->deltas.size() > MAX_HISTORY) {
            this->deltas.pop_front();
        }
        this->deltas.push_back(deltasFromBlock);
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
    string bestHost = "";
    int bestCount = 0;
    for(auto host : this->hosts) {
        try {
            int curr = getCurrentBlockCount(host);
            if (curr > bestCount) {
                bestCount = curr;
                bestHost = host;
            }
        } catch (...) {
            continue;
        }
    }

    this->targetBlockCount = bestCount;

    int startCount = this->numBlocks + 1;

    if (startCount > this->targetBlockCount) {
        return SUCCESS;
    }

    // download any remaining blocks
    for(int i = startCount; i <= this->targetBlockCount; i++) {
        json block = getBlockData(bestHost, i);
        Block toAdd(block);
        ExecutionStatus addResult = this->addBlock(toAdd);
        if (addResult != SUCCESS) {
            // if this chain sync fails, pop off a few blocks before trying again
            for(int j = this->deltas.size() - 1; j <=0; j--) {
                Executor::Rollback(this->ledger, this->deltas.back());
                this->deltas.pop_back();
                this->numBlocks--;
            }
            return addResult;
        }
    }
    return SUCCESS;
}


