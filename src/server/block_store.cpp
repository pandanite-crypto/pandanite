#include <iostream>
#include <thread>
#include "../core/transaction.hpp"
#include "../core/logger.hpp"
#include "block_store.hpp"
using namespace std;

BlockStore::BlockStore() {
}

void BlockStore::setBlockCount(size_t count) {
    this->blockCount = count;
}

size_t BlockStore::getBlockCount() {
    return this->blockCount;
}

void BlockStore::setTotalWork(Bigint count) {
    this->totalWork = count;
}

Bigint BlockStore::getTotalWork() {
    return this->totalWork;
}

bool BlockStore::hasBlock(uint32_t blockId) {
    return this->blocks.find(blockId) != this->blocks.end();
}

BlockHeader BlockStore::getBlockHeader(uint32_t blockId) {
    return this->blocks[blockId].serialize();
}

vector<TransactionInfo> BlockStore::getBlockTransactions(BlockHeader& block) {
    Block& b = this->blocks[block.id];
    vector<TransactionInfo> transactions;
    for(auto t : b.getTransactions()) {
        transactions.push_back(t.serialize());
    }
    return std::move(transactions);
}

std::pair<uint8_t*, size_t> BlockStore::getRawData(uint32_t blockId) {
    Block& b = this->blocks[blockId];
    BlockHeader block = b.serialize();
    size_t numBytes = BLOCKHEADER_BUFFER_SIZE + (TRANSACTIONINFO_BUFFER_SIZE * block.numTransactions);
    char* buffer = (char*)malloc(numBytes);
    blockHeaderToBuffer(block, buffer);
    char* transactionBuffer = buffer + BLOCKHEADER_BUFFER_SIZE;
    char* currTransactionPtr = transactionBuffer;
    for(auto t : b.getTransactions()) {
        TransactionInfo txinfo = t.serialize();
        transactionInfoToBuffer(txinfo, currTransactionPtr);
        currTransactionPtr += TRANSACTIONINFO_BUFFER_SIZE;
    }
    return std::pair<uint8_t*, size_t>((uint8_t*)buffer, numBytes);
}

Block BlockStore::getBlock(uint32_t blockId) {
    return this->blocks[blockId];
}

void BlockStore::setBlock(Block& block) {
    uint32_t blockId = block.getId();
    this->blocks[blockId] = block;
}

