#pragma once
#include <string>
#include "leveldb/db.h"
#include "../core/transaction.hpp"
#include "data_store.hpp"
using namespace std;


class TransactionStore : public DataStore {
    public:
        TransactionStore();
        bool hasTransaction(Transaction &t);
        void insertTransaction(Transaction& t);
};