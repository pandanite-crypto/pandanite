#pragma once
#include <string>
#include <map>
#include "../core/transaction.hpp"
using namespace std;


class TransactionStore {
    public:
        TransactionStore();
        bool hasTransaction(const Transaction &t);
        uint32_t blockForTransaction(Transaction &t);
        void insertTransaction(Transaction& t, uint32_t blockId);
        void removeTransaction(Transaction & t);
        void clear();
    private:
        map<SHA256Hash, uint32_t> seen;
};