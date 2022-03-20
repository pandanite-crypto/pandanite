#include <iostream>
#include <thread>
#include "../core/crypto.hpp"
#include "ledger.hpp"
using namespace std;


Ledger::Ledger() {
}



bool Ledger::hasWallet(const PublicWalletAddress& wallet) {
    return ledger.find(wallet) != ledger.end();
}

void Ledger::createWallet(const PublicWalletAddress& wallet) {
    if(this->hasWallet(wallet)) throw std::runtime_error("Wallet exists");
    ledger[wallet] = 0;
}

void Ledger::setWalletValue(const PublicWalletAddress& wallet, TransactionAmount amount) {
    ledger[wallet] = amount;
}

TransactionAmount Ledger::getWalletValue(const PublicWalletAddress& wallet) {
    return ledger[wallet];
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