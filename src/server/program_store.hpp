#pragma once
#include <string>
#include "leveldb/db.h"
#include "data_store.hpp"
#include "../core/program.hpp"
using namespace std;


class ProgramStore : public DataStore {
    public:
        ProgramStore();
        bool hasProgram(const ProgramID& programId);
        Program getProgram(const ProgramID& programId);
        void insertProgram(const Program& p);
        void removeProgram(const Program& p);
};