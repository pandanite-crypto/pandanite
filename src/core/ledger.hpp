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
        size_t size();
        void closeDB();
        void deleteDB();
    protected:
        void setWalletValue(const PublicWalletAddress& wallet, TransactionAmount amount);
        leveldb::DB *db;
        size_t _size;
        string path;
};