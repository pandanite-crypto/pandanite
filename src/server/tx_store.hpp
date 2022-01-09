#pragma once
#include <string>
#include "leveldb/db.h"
#include "../core/transaction.hpp"
#include "data_store.hpp"
using namespace std;


class TransactionStore : public DataStore {
    public:
        TransactionStore();
        bool hasTransaction(const Transaction &t);
        uint32_t blockForTransaction(Transaction &t);
        void insertTransaction(Transaction& t, uint32_t blockId);
        void removeTransaction(Transaction & t);
};