#include "data_store.hpp"
#include <thread>

#ifdef _WIN32
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

DataStore::DataStore() {
    this->db = NULL;
}

void DataStore::closeDB() {
    delete db;
}

string DataStore::getPath() {
    return this->path;
}

void DataStore::clear() {
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        string key = it->key().ToString();
        leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
        if(!status.ok()) throw std::runtime_error("Could not clear data store : " + status.ToString());
    }
    assert(it->status().ok()); 
    delete it;
}

void DataStore::deleteDB() {
    leveldb::Options options;
    leveldb::Status status = leveldb::DestroyDB(this->path, options);
#ifdef _WIN32
    filesystem::remove_all(this->path); 
#else
    experimental::filesystem::remove_all(this->path); 
#endif
    if(!status.ok()) throw std::runtime_error("Could not close DataStore db : " + status.ToString());
}

void DataStore::init(string path) {
    if (this->db) {
        this->closeDB();
    }
    this->path = path;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, path, &this->db);
    if(!status.ok()) throw std::runtime_error("Could not write DataStore db : " + status.ToString());
}