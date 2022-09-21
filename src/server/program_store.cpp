#include "program_store.hpp"


ProgramStore::ProgramStore() {
}


bool ProgramStore::hasProgram(const ProgramID& programId) {
    leveldb::Slice key = leveldb::Slice((const char*) programId.data(), programId.size());
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    return (status.ok());
}

Program ProgramStore::getProgram(const ProgramID& programId) {
    leveldb::Slice key = leveldb::Slice((const char*) programId.data(), programId.size());
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    if (!status.ok()) throw std::runtime_error("Could not find program");
    vector<uint8_t> bytes;
    const char* c_str = value.c_str();
    std::copy(c_str, c_str + value.size(), std::back_inserter(bytes));
    return Program(bytes);
}

void ProgramStore::insertProgram(const Program& p){
    leveldb::Slice key = leveldb::Slice((const char*) p.getId().data(), p.getId().size());
    vector<uint8_t> byteCode = p.getByteCode();
    leveldb::Slice slice = leveldb::Slice((const char*)byteCode.data(), byteCode.size());
    leveldb::Status status = db->Put(leveldb::WriteOptions(), key, slice);
    if(!status.ok()) throw std::runtime_error("Could not write program to tx db : " + status.ToString());
}

void ProgramStore::removeProgram(const Program& p) {
    leveldb::Slice key = leveldb::Slice((const char*) p.getId().data(), p.getId().size());
    leveldb::Status status = db->Delete(leveldb::WriteOptions(), key);
    if(!status.ok()) throw std::runtime_error("Could not remove program from tx db : " + status.ToString());
}