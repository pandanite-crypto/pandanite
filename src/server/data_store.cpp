#include "data_store.hpp"
#include <thread>
#include <experimental/filesystem>

DataStore::DataStore() {
    this->db = NULL;
}

void DataStore::closeDB() {
    delete db;
}

void DataStore::deleteDB() {
    leveldb::Options options;
    leveldb::Status status = leveldb::DestroyDB(this->path, options);
    experimental::filesystem::remove_all(this->path); 
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