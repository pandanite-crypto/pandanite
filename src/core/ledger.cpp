#include "ledger.hpp"
#include "crypto.hpp"
#include <iostream>
#include <thread>
#include <experimental/filesystem>
using namespace std;


Ledger::Ledger() {
    this->db = NULL;
}

void Ledger::deleteDB() {
    leveldb::Options options;
    leveldb::Status status = leveldb::DestroyDB(this->path, options);
    experimental::filesystem::remove_all(this->path); 
    if(!status.ok()) throw std::runtime_error("Could not close BlockStore db : " + status.ToString());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void Ledger::init(string path) {
    if (this->db) {
        this->closeDB();
    }
    this->path = path;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    this->taxRate = 0;
    this->totalTax = 0;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, path, &this->db);
    if(!status.ok()) throw std::runtime_error("Could not write ledger db : " + status.ToString());
}

void Ledger::closeDB() {
    delete db;
}

void Ledger::recordTax(TransactionAmount amt) {
    this->totalTax += amt;
}

void Ledger::resetTax() {
    this->totalTax = 0;
}

TransactionAmount Ledger::getTaxCollected() {
    return this->totalTax;
}

void Ledger::payoutTaxes() {
    TransactionAmount total = this->getTaxCollected();
    TransactionAmount perWallet = total / this->taxRecepients.size();
    for(auto wallet : this->taxRecepients) {
        TransactionAmount value = this->getWalletValue(wallet);
        this->setWalletValue(wallet, value + perWallet);
    }
    this->resetTax();
}

void Ledger::setTaxRate(double rate) {
    this->taxRate = rate;
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

void Ledger::createWallet(const PublicWalletAddress& wallet) {
    if(this->hasWallet(wallet)) throw std::runtime_error("Wallet exists");
    this->setWalletValue(wallet, 0);
}

void Ledger::setWalletValue(const PublicWalletAddress& wallet, TransactionAmount amount) {
    if (!this->hasWallet(wallet)) {
        // check if this is a founding wallet:
        if (isFounderWalletPossible(wallet) && this->taxRecepients.size() < TOTAL_FOUNDERS) {
            this->taxRecepients.insert(wallet);
        }
        _size++;
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

void Ledger::revertSend(const PublicWalletAddress& wallet, TransactionAmount amt) {
    TransactionAmount value = this->getWalletValue(wallet);
    this->setWalletValue(wallet, value + amt);
}

void Ledger::deposit(const PublicWalletAddress& wallet, TransactionAmount amt) {
    TransactionAmount value = this->getWalletValue(wallet);
    TransactionAmount tax = this->taxRate * amt;
    this->recordTax(tax);
    this->setWalletValue(wallet, value + amt - tax);
}

void Ledger::revertDeposit(PublicWalletAddress to, TransactionAmount amt) {
    TransactionAmount value = this->getWalletValue(to);
    TransactionAmount tax = this->taxRate * amt;
    TransactionAmount deposited = amt - tax;
    this->recordTax(-tax);
    this->setWalletValue(to, value - deposited);
}