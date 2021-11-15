#include "block_store.hpp"
#include "crypto.hpp"
#include "transaction.hpp"
#include <iostream>
#include <thread>
#include <experimental/filesystem>
using namespace std;


BlockStore::BlockStore() {
    this->db = NULL;
}

void BlockStore::init(string path) {
    if (this->db) {
        this->clearDB();
    }
    this->path = path;
    experimental::filesystem::remove_all(this->path);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    this->numBlocks = 0;
    leveldb::Options options;
    options.create_if_missing = true;
    options.error_if_exists = true;
    leveldb::Status status = leveldb::DB::Open(options, path, &this->db);
    if(!status.ok()) throw std::runtime_error("Could not write BlockStore db : " + status.ToString());
}

bool BlockStore::hasBlock(int blockId) {
    leveldb::Slice key = leveldb::Slice((const char*) &blockId, sizeof(int));
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    return (status.ok());
}

Block BlockStore::getBlock(int blockId) {
    leveldb::Slice key = leveldb::Slice((const char*) &blockId, sizeof(int));
    string valueStr;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &valueStr);
    if(!status.ok()) throw std::runtime_error("Could not read block header from BlockStore db : " + status.ToString());
    
    BlockHeader value;
    memcpy(&value, valueStr.c_str(), sizeof(BlockHeader));
    
    vector<Transaction> transactions;
    for(int i = 0; i < value.numTransactions; i++) {
        int transactionId[2];
        transactionId[0] = blockId;
        transactionId[1] = i;
        leveldb::Slice key = leveldb::Slice((const char*) transactionId, 2*sizeof(int));
        string valueStr;
        leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &valueStr);
        if(!status.ok()) throw std::runtime_error("Could not read transaction from BlockStore db : " + status.ToString());
        TransactionInfo t;
        memcpy(&t, valueStr.c_str(), sizeof(TransactionInfo));
        transactions.push_back(Transaction(t));
    }
    Block ret(value, transactions);
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


void BlockStore::clearDB() {
    delete db;
    leveldb::Options options;
    leveldb::Status status = leveldb::DestroyDB(this->path, options);
    experimental::filesystem::remove_all(this->path); 
    if(!status.ok()) throw std::runtime_error("Could not close BlockStore db : " + status.ToString());
}

BlockStore::~BlockStore() {
    if(this->db) this->clearDB();
}

