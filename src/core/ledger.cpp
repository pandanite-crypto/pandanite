#include "ledger.hpp"
#include "crypto.hpp"
#include <iostream>
#include <filesystem>
using namespace std;


Ledger::Ledger() {
    this->db = NULL;
}

void Ledger::init(string path) {
    if (this->db) {
        this->clearDB();
    }
    this->path = path;
    leveldb::Options options;
    options.create_if_missing = true;
    options.error_if_exists = true;
    leveldb::Status status = leveldb::DB::Open(options, path, &this->db);
    if(!status.ok()) throw std::runtime_error("Could not write ledger db : " + status.ToString());
}

void Ledger::clearDB() {
    delete db;
    leveldb::Options options;
    leveldb::Status status = leveldb::DestroyDB(this->path, options);
    std::filesystem::remove(this->path); 
    if(!status.ok()) throw std::runtime_error("Could not close ledger db : " + status.ToString());
}

Ledger::~Ledger() {
    if(this->db) this->clearDB();
}

leveldb::Slice walletToSlice(const PublicWalletAddress& w) {
    leveldb::Slice s2 = leveldb::Slice((const char*) w.data(), w.size());
    return s2;
}

leveldb::Slice amountToSlice(const TransactionAmount& t) {
    leveldb::Slice s2 = leveldb::Slice((const char*) &t, sizeof(TransactionAmount));
    return s2;
}

size_t Ledger::size() {
    return _size;
}

bool Ledger::hasWallet(const PublicWalletAddress& wallet) {
    std::string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), walletToSlice(wallet), &value);
    return (status.ok());
}

void Ledger::setWalletValue(const PublicWalletAddress& wallet, TransactionAmount amount) {
    if (!this->hasWallet(wallet)) {
        _size++;
        leveldb::Slice slice = leveldb::Slice((const char*)&_size, sizeof(_size));
        string key = "num_wallets";
        db->Put(leveldb::WriteOptions(), key, slice);
    }
    leveldb::Status status = db->Put(leveldb::WriteOptions(), walletToSlice(wallet), amountToSlice(amount));
    if (!status.ok()) throw std::runtime_error("Write failed: " + status.ToString());
}

TransactionAmount Ledger::getWalletValue(const PublicWalletAddress& wallet) {
    std::string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), walletToSlice(wallet), &value);
    if(!status.ok()) throw std::runtime_error("Tried fetching wallet value for non-existant wallet");
    return *((TransactionAmount*)value.c_str());
}

void Ledger::withdraw(const PublicWalletAddress& wallet, TransactionAmount amt) {
    TransactionAmount value = this->getWalletValue(wallet);
    value -= amt;
    this->setWalletValue(wallet, value);
}

void Ledger::deposit(const PublicWalletAddress& wallet, TransactionAmount amt) {
    TransactionAmount value = this->getWalletValue(wallet);
    value += amt;
    this->setWalletValue(wallet, value);
}