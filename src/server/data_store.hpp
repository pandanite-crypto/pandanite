#pragma once
#include <string>
#include "leveldb/db.h"
using namespace std;

class DataStore {
    public:
        DataStore();
        void init(string path);
        void deleteDB();
        void closeDB();
    protected:
        leveldb::DB* db;
        string path;
};