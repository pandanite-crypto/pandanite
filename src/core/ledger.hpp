#pragma once
#include "common.hpp"
#include "leveldb/db.h"

class Ledger {
    public:
        Ledger();
        ~Ledger();
        void init(string path);
        bool hasWallet(const PublicWalletAddress& wallet);
        void setWalletValue(const PublicWalletAddress& wallet, TransactionAmount amount);
        TransactionAmount getWalletValue(const PublicWalletAddress& wallet);
        void withdraw(const PublicWalletAddress& wallet, TransactionAmount amt);
        void deposit(const PublicWalletAddress& wallet, TransactionAmount amt);
        size_t size();
    protected:
        void clearDB();
        leveldb::DB *db;
        size_t _size;
        string path;
};