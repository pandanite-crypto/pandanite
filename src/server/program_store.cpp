#include "program_store.hpp"


ProgramStore::ProgramStore(json config) {
    this->config = config;
}


bool ProgramStore::hasProgram(const ProgramID& programId) {
    leveldb::Slice key = leveldb::Slice((const char*) programId.data(), programId.size());
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    return (status.ok());
}

vector<ProgramID> ProgramStore::getProgramIds() {
    SHA256Hash start = NULL_SHA256_HASH;
    SHA256Hash end;
    for(int i = 0; i < end.size(); i++) {
        end[i] = 255;
    }
    auto startSlice = leveldb::Slice((const char*) start.data(), start.size());
    auto endSlice = leveldb::Slice((const char*) end.data(), end.size());

    // Get a database iterator
    std::shared_ptr<leveldb::Iterator> it(db->NewIterator(leveldb::ReadOptions()));

    vector<SHA256Hash> ret;
    for(it->Seek(startSlice); it->Valid() && it->key().compare(endSlice) < 0; it->Next()) {                
        leveldb::Slice keySlice(it->key());
        SHA256Hash txid;
        memcpy(txid.data(), keySlice.data(), 32);
        ret.push_back(txid);
    }
    return std::move(ret);
}

std::shared_ptr<Program> ProgramStore::getProgram(const ProgramID& programId) {
    leveldb::Slice key = leveldb::Slice((const char*) programId.data(), programId.size());
    string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(),key, &value);
    if (!status.ok()) throw std::runtime_error("Could not find program");
    vector<uint8_t> bytes;
    const char* c_str = value.c_str();
    std::copy(c_str, c_str + value.size(), std::back_inserter(bytes));
    return std::make_shared<Program>(bytes, this->config);
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