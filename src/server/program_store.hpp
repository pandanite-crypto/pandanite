#pragma once
#include <string>
#include "leveldb/db.h"
#include "data_store.hpp"
#include "program.hpp"
using namespace std;


class ProgramStore : public DataStore {
    public:
        ProgramStore(json config);
        bool hasProgram(const ProgramID& programId);
        std::shared_ptr<Program> getProgram(const ProgramID& programId);
        void insertProgram(const Program& p);
        void removeProgram(const Program& p);
        vector<ProgramID> getProgramIds();
    protected:
        json config;
};