#include "program.hpp"

Program::Program(vector<uint8_t>& byteCode) {
    this->byteCode = byteCode;
    this->id = SHA256((char*)byteCode.data(), byteCode.size());
}

Program::Program(json obj) {
    this->byteCode = hexDecode(obj["byteCode"]);
    this->id = SHA256((char*)byteCode.data(), byteCode.size());
}

vector<uint8_t> Program::getByteCode() const {
    return this->byteCode;
}

json Program::toJson() const{
    json obj;
    obj["byteCode"] = hexEncode((char*)this->byteCode.data(), this->byteCode.size());
    return obj;
}

ProgramID Program::getId() const{
    return this->id;
}

bool operator==(const Program& a, const Program& b) {
    if(a.id != b.id) return false;
    //if(a.byteCode != b.byteCode) return false;
    return true;
}