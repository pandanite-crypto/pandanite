#pragma once
#include "common.hpp"
#include "leveldb/db.h"
#include <set>
using namespace std;

class Ledger {
    public:
        Ledger();
        void init(string path);
        bool hasWallet(const PublicWalletAddress& wallet);
        TransactionAmount getWalletValue(const PublicWalletAddress& wallet);
        void createWallet(const PublicWalletAddress& wallet);
        void withdraw(const PublicWalletAddress& wallet, TransactionAmount amt);
        void revertSend(const PublicWalletAddress& wallet, TransactionAmount amt);
        void revertDeposit(PublicWalletAddress to, TransactionAmount amt);
        void deposit(const PublicWalletAddress& wallet, TransactionAmount amt);
        void recordTax(TransactionAmount amt);
        void resetTax();
        void setTaxRate(double rate);
        TransactionAmount getTaxCollected();
        void payoutTaxes();
        size_t size();
        void closeDB();
        void deleteDB();
    protected:
        void setWalletValue(const PublicWalletAddress& wallet, TransactionAmount amount);
        set<PublicWalletAddress> taxRecepients;
        double taxRate;
        TransactionAmount totalTax;
        leveldb::DB *db;
        size_t _size;
        string path;
};