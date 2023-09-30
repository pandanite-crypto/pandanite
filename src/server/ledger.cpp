#include <iostream>
#include <thread>
#include "../core/crypto.hpp"
#include "ledger.hpp"
using namespace std;


Ledger::Ledger() {
}

leveldb::Slice walletToSlice(const PublicWalletAddress& w) {
    leveldb::Slice s2 = leveldb::Slice((const char*) w.data(), w.size());
    return s2;
}

leveldb::Slice amountToSlice(const TransactionAmount& t) {
    leveldb::Slice s2 = leveldb::Slice((const char*) &t, sizeof(TransactionAmount));
    return s2;
}

bool Ledger::hasWallet(const PublicWalletAddress& wallet) const{
    std::string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), walletToSlice(wallet), &value);
    return (status.ok());
}

void Ledger::createWallet(const PublicWalletAddress& wallet) {
    if(this->hasWallet(wallet)) throw std::runtime_error("Wallet exists");
    this->setWalletValue(wallet, 0);
}

void Ledger::setWalletValue(const PublicWalletAddress& wallet, TransactionAmount amount) {
    leveldb::Status status = db->Put(leveldb::WriteOptions(), walletToSlice(wallet), amountToSlice(amount));
    if (!status.ok()) throw std::runtime_error("Write failed: " + status.ToString());
}

TransactionAmount Ledger::getWalletValue(const PublicWalletAddress& wallet) const{
    std::string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), walletToSlice(wallet), &value);
    if(!status.ok()) throw std::runtime_error("Tried fetching wallet value for non-existant wallet");
    TransactionAmount amt{ *((TransactionAmount*)value.c_str())};

    // set overflow values to 0
    constexpr size_t hundredMillions{100ull* 1000ull*1000ull*10000ull};
    if (amt > hundredMillions)
        return 0;
    return amt;
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
    this->setWalletValue(wallet, value + amt);
}

void Ledger::revertDeposit(PublicWalletAddress to, TransactionAmount amt) {
    TransactionAmount value = this->getWalletValue(to);
    this->setWalletValue(to, value - amt);
}
