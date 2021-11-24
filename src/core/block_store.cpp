#include "block_store.hpp"
#include "crypto.hpp"
#include "transaction.hpp"
#include "logger.hpp"
#include <iostream>
#include <thread>
#include <experimental/filesystem>
using namespace std;


BlockStore::BlockStore() {
    this->db = NULL;
}

void BlockStore::closeDB() {
    delete db;
}

void BlockStore::deleteDB() {
    leveldb::Options options;
    leveldb::Status status = leveldb::DestroyDB(this->path, options);
    experimental::filesystem::remove_all(this->path); 
    if(!status.ok()) throw std::runtime_error("Could not close BlockStore db : " + status.ToString());
}

void BlockStore::init(string path) {
    if (this->db) {
        this->closeDB();
    }
    this->path = path;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    this->numBlocks = 0;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, path, &this->db);
    if(!status.ok()) throw std::runtime_error("Could not write BlockStore db : " + status.ToString());
}

void BlockStore::setBlockCount(size_t count) {
    string countKey = "BLOCK_COUNT";
    size_t num = count;
    leveldb::Slice key = leveldb::Slice(countKey);
    leveldb::Slice slice = leveldb::Slice((const char*)&num, sizeof(size_t));
    leveldb::WriteOptions write_options;
    write_options.sync = true;
    leveldb::Status status = db->Put(write_options, key, slice);
    if(!status.ok()) throw std::runtime_error("Could not write block count to DB : " + status.ToString());
}

size_t BlockStore::getBlockCount() {
    string countKey = "BLOCK_COUNT";
    leveldb::Slice key = leveldb::Slice(countKey);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    if(!status.ok()) throw std::runtime_error("Could not read block count from DB : " + status.ToString());
    size_t ret = *((size_t*)value.c_str());
    return ret;
}

bool BlockStore::hasBlockCount() {
    string countKey = "BLOCK_COUNT";
    leveldb::Slice key = leveldb::Slice(countKey);
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    return (status.ok());
}

bool BlockStore::hasBlock(int blockId) {
    leveldb::Slice key = leveldb::Slice((const char*) &blockId, sizeof(int));
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    return (status.ok());
}

BlockHeader BlockStore::getBlockHeader(int blockId) {
    leveldb::Slice key = leveldb::Slice((const char*) &blockId, sizeof(int));
    string valueStr;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &valueStr);
    if(!status.ok()) throw std::runtime_error("Could not read block header from BlockStore db : " + status.ToString());
    
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

std::pair<char*, size_t> BlockStore::getRawData(int blockId) {
    BlockHeader block = this->getBlockHeader(blockId);
    size_t numBytes = sizeof(BlockHeader) + (sizeof(TransactionInfo) * block.numTransactions);
    char* buffer = (char*)malloc(numBytes);
    memcpy(buffer,&block, sizeof(BlockHeader));
    char* transactionBuffer = buffer + sizeof(BlockHeader);
    char* currTransactionPtr = transactionBuffer;
    for(int i = 0; i < block.numTransactions; i++) {
        int idx = i;
        int transactionId[2];
        transactionId[0] = blockId;
        transactionId[1] = idx;
        leveldb::Slice key = leveldb::Slice((const char*) transactionId, 2*sizeof(int));
        string value;
        leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
        memcpy(currTransactionPtr, value.c_str(), sizeof(TransactionInfo));
        if (!status.ok()) throw std::runtime_error("Could not read transaction from BlockStore db : " + status.ToString());
        currTransactionPtr += sizeof(TransactionInfo);
    }
    return std::pair<char*, size_t>(buffer, numBytes);
}

Block BlockStore::getBlock(int blockId) {
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
    int blockId = block.getId();
    leveldb::Slice key = leveldb::Slice((const char*) &blockId, sizeof(int));
    BlockHeader blockStruct = block.serialize();
    leveldb::Slice slice = leveldb::Slice((const char*)&blockStruct, sizeof(BlockHeader));
    leveldb::Status status = db->Put(leveldb::WriteOptions(), key, slice);
    if(!status.ok()) throw std::runtime_error("Could not write block to BlockStore db : " + status.ToString());
    for(int i = 0; i < block.getTransactions().size(); i++) {
        int transactionId[2];
        transactionId[0] = blockId;
        transactionId[1] = i;
        TransactionInfo t = block.getTransactions()[i].serialize();
        leveldb::Slice key = leveldb::Slice((const char*) transactionId, 2*sizeof(int));
        leveldb::Slice slice = leveldb::Slice((const char*)&t, sizeof(TransactionInfo));
        leveldb::Status status = db->Put(leveldb::WriteOptions(), key, slice);
        if(!status.ok()) throw std::runtime_error("Could not write transaction to BlockStore db : " + status.ToString());
    }
}

