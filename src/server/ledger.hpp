#pragma once
#include <set>
#include <cstdint>
#include "leveldb/db.h"
#include "../core/common.hpp"
#include "data_store.hpp"
using namespace std;

class Ledger : public DataStore {
    public:
        Ledger();
        bool hasWallet(const PublicWalletAddress& wallet) const;
        TransactionAmount getWalletValue(const PublicWalletAddress& wallet) const;
        void createWallet(const PublicWalletAddress& wallet);
        void withdraw(const PublicWalletAddress& wallet, TransactionAmount amt);
        void revertSend(const PublicWalletAddress& wallet, TransactionAmount amt);
        void revertDeposit(PublicWalletAddress to, TransactionAmount amt);
        void deposit(const PublicWalletAddress& wallet, TransactionAmount amt);
    protected:
        void setWalletValue(const PublicWalletAddress& wallet, TransactionAmount amount);
};
