#pragma once
#include <set>
#include "leveldb/db.h"
#include "../core/common.hpp"
#include "data_store.hpp"
using namespace std;

class PufferfishCache : public DataStore {
    public:
        PufferfishCache();
        bool hasHash(const SHA256Hash& hash);
        SHA256Hash getHash(const SHA256Hash& hash);
        void setHash(const SHA256Hash& input, const SHA256Hash& hash);
};