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
        cout<<"synchronizing chain"<<endl;
        ExecutionStatus valid;
        blockchain.acquire();
        valid = blockchain.startChainSync();
        if (valid != SUCCESS) {
            blockchain.resetChain();
        } else {
            cout<<"chain in sync, node count: " <<blockchain.getBlockCount()<<endl;
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
    this->chain.clear();
    for(size_t i = 0; i <this->lastHash.size(); i++) this->lastHash[i] = 0;
    json data = readJsonFromFile(GENESIS_FILE_PATH);
    Block genesis(data);
    this->difficulty = genesis.getDifficulty();
    this->targetBlockCount = 1;
    ExecutionStatus status = this->addBlock(genesis);
    if (status != SUCCESS) {
        cout<<"Could not load genesis block"<<endl;
        cout<<executionStatusAsString(status)<<endl;
    }
}

void BlockChain::sync(string host) {
    this->host = host;
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
    return this->chain.size();
}

Block BlockChain::getBlock(int idx) {
    if (idx < 0 || idx > this->chain.size()) throw std::runtime_error("Invalid block");
    return this->chain[idx-1];
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

void BlockChain::updateDifficulty(Block& block) {
    if(block.getId() % DIFFICULTY_RESET_FREQUENCY == 0) {
        // compute the new difficulty score based on average block time
        double average = 0;
        int total = 0;
        // ignore genesis block time
        int first = max((size_t) 3, this->chain.size() - DIFFICULTY_RESET_FREQUENCY);
        int last = this->chain.size() - 1;
        for(int i = last; i >= first; i--) {
            int currTs = (int)this->chain[i-1].getTimestamp();
            int lastTs = (int)this->chain[i-2].getTimestamp();
            average += (double)(currTs - lastTs);
            total++;
        }
        average /= total;
        double ratio = average/DESIRED_BLOCK_TIME_SEC;
        int delta = -round(log2(ratio));
        this->difficulty += delta;
        this->difficulty = min(max(this->difficulty, MIN_DIFFICULTY), MAX_DIFFICULTY);
        cout<<"Average block time: "<<average<<endl;
        cout<<"Updated difficulty: "<<this->difficulty<<endl;
    }
}

int BlockChain::getDifficulty() {
    return this->difficulty;
}


ExecutionStatus BlockChain::addBlock(Block& block) {
    // check difficulty + nonce
    if (block.getId() != this->chain.size() + 1) return INVALID_BLOCK_ID;
    if (block.getDifficulty() != this->difficulty) return INVALID_DIFFICULTY;
    if (!block.verifyNonce(this->getLastHash())) return INVALID_NONCE;
    LedgerState deltasFromBlock;
    ExecutionStatus status = Executor::ExecuteBlock(block, this->ledger, deltasFromBlock);
    
    if (status != SUCCESS) {
        //revert ledger
        Executor::Rollback(this->ledger, deltasFromBlock);
    } else {
        this->chain.push_back(block);
        this->lastHash = block.getHash(concatHashes(this->getLastHash(), block.getNonce()));
        this->updateDifficulty(block);
    }
    return status;
}

ExecutionStatus BlockChain::startChainSync() {
    this->targetBlockCount = getCurrentBlockCount(host);

    int startCount = this->chain.size() + 1;

    if (startCount > this->targetBlockCount) {
        return SUCCESS;
    }

    // download any remaining blocks
    for(int i = startCount; i <= this->targetBlockCount; i++) {
        // get data for block by polling at random
        json block = getBlockData(this->host, i);
        Block toAdd(block);
        ExecutionStatus addResult = this->addBlock(toAdd);
        if (addResult != SUCCESS) {
            cout<<"Failed to add block:" << executionStatusAsString(addResult)<<endl;
            return addResult;
        }
    }
    return SUCCESS;
}

bool BlockChain::isLoaded() {
    return this->chain.size() == this->targetBlockCount;
};

