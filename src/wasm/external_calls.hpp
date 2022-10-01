#pragma once


void print_str(char* str, int sz);
void print(const char* str) {
    print_str((char*)str, strlen(str));
}

// uint32/uint64
void setUint32(char* name, uint32_t value, uint32_t index = 0);
void setUint64(char* key, uint64_t value, uint32_t index = 0);
uint64_t getUint64(char* key, uint32_t index = 0);
uint32_t getUint32(char* key, uint32_t index = 0);

// arr modifications
void pop(const char* key);
void removeItem(const char* key, uint32_t idx);
uint32_t itemCount(const char* key);

//wallets
void _setWallet(char* key, char* val, uint32_t idx = 0);
void _getWallet(char* key, char* buf, uint32_t sz, uint32_t index = 0);
PublicWalletAddress getWallet(char* key, uint32_t idx = 0) {
    char buf[50];
    _getWallet(key, buf, 50, idx);
    return stringToWalletAddress(string(buf));
}

void setWallet(char* key, const PublicWalletAddress& val, uint32_t idx = 0) {
    string s = walletAddressToString(val);
    char* buf = (char*) s.c_str();
    _setWallet(key, buf, idx);
}

//sha256
void _getSha256(char* key, char* buf, uint32_t sz, uint32_t index = 0);
void _setSha256(char* key, char* val, uint32_t idx = 0);
SHA256Hash getSha256(char* key, uint32_t idx = 0) {
    char buf[64];
    _getSha256(key, buf, 64, idx);
    string s = string(buf);
    return stringToSHA256(s);
}

void setSha256(char* key, const SHA256Hash& val, uint32_t idx = 0) {
    string s = SHA256toString(val);
    char* buf = (char*)s.c_str();
    _setSha256(key, buf, idx);
}

// bytes
uint32_t _getBytes(const char* key, char* buf, uint32_t sz, uint32_t index = 0);
void _setBytes(char* key, char* val, uint32_t sz, uint32_t idx = 0);
void setBytes(char* key, const vector<uint8_t>& val, uint32_t idx = 0) {
    _setBytes(key, (char*) val.data(), val.size(), idx);
}

vector<uint8_t> getBytes(char* key, uint32_t idx = 0) {
    char buf[4096];
    uint32_t sz = _getBytes(key, buf, 4096, idx);
    vector<uint8_t> bytes;
    for(int i = 0; i < sz; i++) bytes.push_back(buf[i]);
    return std::move(bytes);
}

//bigint
void _setBigint(char* key, char* val, uint32_t sz, uint32_t idx = 0);
uint32_t _getBigint( char* key, char* buf, uint32_t sz, uint32_t index = 0);
void setBigint(char* key, const Bigint& val, uint32_t idx = 0) {
    vector<uint8_t> buf = val.toBuffer();
    _setBigint(key, (char*) buf.data(), buf.size(), idx);
}

Bigint getBigint(char* key, uint32_t idx = 0) {
    char buf[4096];
    uint32_t sz = _getBigint(key, buf, 4096, idx);
    return Bigint::fromBuffer(buf, sz);
}

void setReturnValue(char* val, uint32_t sz);






