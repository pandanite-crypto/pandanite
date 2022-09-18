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

leveldb::Slice walletProgramStatus(const PublicWalletAddress& w, std::array<uint8_t,26>& walletProgramStatusKey) {
    // copy wallet address into byte array
    std::copy_n(w.begin(), w.size(), walletProgramStatusKey.begin());
    walletProgramStatusKey[25] = 255; // append flag byte to indicate it is a program status
    leveldb::Slice s2 = leveldb::Slice((const char*) walletProgramStatusKey.data(), walletProgramStatusKey.size());
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

bool Ledger::hasWalletProgram(const PublicWalletAddress& wallet) const {
    std::string value;
    std::array<uint8_t, 26> buf; // TODO: This is ugly, but leveldb::Slice needs membuffer to point into
    leveldb::Status status = db->Get(leveldb::ReadOptions(), walletProgramStatus(wallet, buf), &value);
    return (status.ok());
}

void Ledger::removeWalletProgram(const PublicWalletAddress& wallet) {
    std::array<uint8_t, 26> buf; // TODO: This is ugly, but leveldb::Slice needs membuffer to point into
    leveldb::Slice key = walletProgramStatus(wallet, buf);
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
    if(!status.ok()) throw std::runtime_error("Could not remove wallet program " + status.ToString());
}

ProgramID Ledger::getWalletProgram(const PublicWalletAddress& wallet) const {
    std::string value;
    std::array<uint8_t, 26> buf; // TODO: This is ugly, but leveldb::Slice needs membuffer to point into
    leveldb::Status status = db->Get(leveldb::ReadOptions(), walletProgramStatus(wallet, buf), &value);
    if (!status.ok()) throw std::runtime_error("No program for wallet");
    return *((ProgramID*)value.c_str());
}

void Ledger::setWalletProgram(const PublicWalletAddress& wallet, const ProgramID& program) {
    std::array<uint8_t, 26> buf; // TODO: This is ugly, but leveldb::Slice needs membuffer to point into
    leveldb::Slice programIdSlice = leveldb::Slice((const char*) program.data(), program.size());
    leveldb::Status status = db->Put(leveldb::WriteOptions(), walletProgramStatus(wallet, buf), programIdSlice);
    if (!status.ok()) throw std::runtime_error("Write failed: " + status.ToString());
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
    this->setWalletValue(wallet, value + amt);
}

void Ledger::revertDeposit(PublicWalletAddress to, TransactionAmount amt) {
    TransactionAmount value = this->getWalletValue(to);
    this->setWalletValue(to, value - amt);
}