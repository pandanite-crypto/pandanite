#include <iostream>
#include <thread>
#include "../core/crypto.hpp"
#include "../core/transaction.hpp"
#include "../core/logger.hpp"
#include "block_store.hpp"
using namespace std;

#define BLOCK_COUNT_KEY "BLOCK_COUNT"
#define TOTAL_WORK_KEY "TOTAL_WORK"

BlockStore::BlockStore() {
}

void BlockStore::setBlockCount(size_t count) {
    string countKey = BLOCK_COUNT_KEY;
    size_t num = count;
    leveldb::Slice key = leveldb::Slice(countKey);
    leveldb::Slice slice = leveldb::Slice((const char*)&num, sizeof(size_t));
    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::Status status = db->Put(write_options, key, slice);
    if(!status.ok()) throw std::runtime_error("Could not write block count to DB : " + status.ToString());
}

size_t BlockStore::getBlockCount() {
    string countKey = BLOCK_COUNT_KEY;
    leveldb::Slice key = leveldb::Slice(countKey);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    if(!status.ok()) throw std::runtime_error("Could not read block count from DB : " + status.ToString());
    size_t ret = *((size_t*)value.c_str());
    return ret;
}

void BlockStore::setTotalWork(Bigint count) {
    string countKey = TOTAL_WORK_KEY;
    string sz = to_string(count);
    leveldb::Slice key = leveldb::Slice(countKey);
    leveldb::Slice slice = leveldb::Slice((const char*)sz.c_str(), sz.size());
    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::Status status = db->Put(write_options, key, slice);
    if(!status.ok()) throw std::runtime_error("Could not write block count to DB : " + status.ToString());
}

Bigint BlockStore::getTotalWork() {
    string countKey = TOTAL_WORK_KEY;
    leveldb::Slice key = leveldb::Slice(countKey);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    if(!status.ok()) throw std::runtime_error("Could not read block count from DB : " + status.ToString());
    Bigint b(value);
    return b;
}

bool BlockStore::hasBlockCount() {
    string countKey = BLOCK_COUNT_KEY;
    leveldb::Slice key = leveldb::Slice(countKey);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    size_t ret = *((size_t*)value.c_str());
    return (status.ok());
}

bool BlockStore::hasBlock(uint32_t blockId) {
    leveldb::Slice key = leveldb::Slice((const char*) &blockId, sizeof(uint32_t));
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    return (status.ok());
}

BlockHeader BlockStore::getBlockHeader(uint32_t blockId) {
    leveldb::Slice key = leveldb::Slice((const char*) &blockId, sizeof(uint32_t));
    string valueStr;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &valueStr);
    if(!status.ok()) throw std::runtime_error("Could not read block header " + to_string(blockId) + " from BlockStore db : " + status.ToString());
    
    BlockHeader value;
    memcpy(&value, valueStr.c_str(), sizeof(BlockHeader));
    return value;
}

vector<TransactionInfo> BlockStore::getBlockTransactions(BlockHeader& block) {
    vector<TransactionInfo> transactions;
    for(int i = 0; i < block.numTransactions; i++) {
        int idx = i;
        int transactionId[2];
        transactionId[0] = block.id;
        transactionId[1] = idx;
        leveldb::Slice key = leveldb::Slice((const char*) transactionId, 2*sizeof(int));
        string valueStr;
        leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &valueStr);
        if(!status.ok()) throw std::runtime_error("Could not read transaction from BlockStore db : " + status.ToString());
        TransactionInfo t;
        memcpy(&t, valueStr.c_str(), sizeof(TransactionInfo));
        transactions.push_back(t);
    }
    return std::move(transactions);
}

std::pair<uint8_t*, size_t> BlockStore::getRawData(uint32_t blockId) {
    BlockHeader block = this->getBlockHeader(blockId);
    size_t numBytes = BLOCKHEADER_BUFFER_SIZE + (TRANSACTIONINFO_BUFFER_SIZE * block.numTransactions);
    char* buffer = (char*)malloc(numBytes);
    blockHeaderToBuffer(block, buffer);
    char* transactionBuffer = buffer + BLOCKHEADER_BUFFER_SIZE;
    char* currTransactionPtr = transactionBuffer;
    for(int i = 0; i < block.numTransactions; i++) {
        int idx = i;
        uint32_t transactionId[2];
        transactionId[0] = blockId;
        transactionId[1] = idx;
        leveldb::Slice key = leveldb::Slice((const char*) transactionId, 2*sizeof(uint32_t));
        string value;
        leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
        TransactionInfo txinfo = *(TransactionInfo*)(value.c_str());
        transactionInfoToBuffer(txinfo, currTransactionPtr);
        if (!status.ok()) throw std::runtime_error("Could not read transaction from BlockStore db : " + status.ToString());
        currTransactionPtr += TRANSACTIONINFO_BUFFER_SIZE;
    }
    return std::pair<uint8_t*, size_t>((uint8_t*)buffer, numBytes);
}

Block BlockStore::getBlock(uint32_t blockId) {
    BlockHeader block = this->getBlockHeader(blockId);
    vector<TransactionInfo> transactionInfo = this->getBlockTransactions(block);
    vector<Transaction> transactions;
    for(auto t : transactionInfo) {
        transactions.push_back(Transaction(t));
    }
    Block ret(block, transactions);
    return ret;
}

void BlockStore::setBlock(Block& block) {
    uint32_t blockId = block.getId();
    leveldb::Slice key = leveldb::Slice((const char*) &blockId, sizeof(uint32_t));
    BlockHeader blockStruct = block.serialize();
    leveldb::Slice slice = leveldb::Slice((const char*)&blockStruct, sizeof(BlockHeader));
    leveldb::Status status = db->Put(leveldb::WriteOptions(), key, slice);
    if(!status.ok()) throw std::runtime_error("Could not write block to BlockStore db : " + status.ToString());
    for(int i = 0; i < block.getTransactions().size(); i++) {
        uint32_t transactionId[2];
        transactionId[0] = blockId;
        transactionId[1] = i;
        TransactionInfo t = block.getTransactions()[i].serialize();
        leveldb::Slice key = leveldb::Slice((const char*) transactionId, 2*sizeof(uint32_t));
        leveldb::Slice slice = leveldb::Slice((const char*)&t, sizeof(TransactionInfo));
        leveldb::Status status = db->Put(leveldb::WriteOptions(), key, slice);
        if(!status.ok()) throw std::runtime_error("Could not write transaction to BlockStore db : " + status.ToString());
    }
}

