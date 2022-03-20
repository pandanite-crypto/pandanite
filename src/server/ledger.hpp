#pragma once
#include <set>
#include <map>
#include "../core/common.hpp"
using namespace std;

class Ledger {
    public:
        Ledger();
        bool hasWallet(const PublicWalletAddress& wallet);
        TransactionAmount getWalletValue(const PublicWalletAddress& wallet);
        void createWallet(const PublicWalletAddress& wallet);
        void withdraw(const PublicWalletAddress& wallet, TransactionAmount amt);
        void revertSend(const PublicWalletAddress& wallet, TransactionAmount amt);
        void revertDeposit(PublicWalletAddress to, TransactionAmount amt);
        void deposit(const PublicWalletAddress& wallet, TransactionAmount amt);
        void clear();
    protected:
        map<PublicWalletAddress, TransactionAmount> ledger;
        void setWalletValue(const PublicWalletAddress& wallet, TransactionAmount amount);
};