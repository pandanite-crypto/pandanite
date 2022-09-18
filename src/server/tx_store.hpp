#pragma once
#include <string>
#include "leveldb/db.h"
#include "../core/transaction.hpp"
#include "data_store.hpp"
using namespace std;


class TransactionStore : public DataStore {
    public:
        TransactionStore();
        bool hasTransaction(const Transaction &t) const;
        uint32_t blockForTransaction(const Transaction &t) const;
        uint32_t blockForTransactionId(SHA256Hash txid) const;
        void insertTransaction(const Transaction& t, uint32_t blockId);
        void removeTransaction(const Transaction & t);
};