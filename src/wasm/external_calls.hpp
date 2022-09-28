#pragma once


void print_str(char* str, int sz);
void setUint32(char* name, uint32_t value);
void setUint32(char* key, uint32_t value);
void setUint64(char* key, uint32_t value);
void pop(char* key);
void removeItem(char* key, uint32_t idx);
uint64_t count(char* key);
uint64_t getUint64(char* key, uint32_t index = 0);
uint32_t getUint32(char* key, uint32_t index = 0);
void _setSha256(char* key, char* val, uint32_t idx = 0);
void _setWallet(char* key, char* val, uint32_t idx = 0);
void _setBytes(char* key, char* val, uint32_t idx = 0);
void _setBigint(char* key, char* val, uint32_t idx = 0);
void _getSha256(char* key, char* buf, uint32_t index = 0);
void _getBigint(char* key, char* buf, uint32_t index = 0);
void _getWallet(char* key, char* buf, uint32_t index = 0);
void _getBytes(char* key, char* buf, uint32_t index = 0);
void setReturnValue(char* val, uint32_t sz);

PublicWalletAddress getWallet(const char* key, uint32_t idx = 0) {
    char buf[4096];
    _getWallet(key, buf, idx);
    return stringToWalletAddress(string(buf));
}

SHA256Hash getSha256(const char* key, uint32_t idx = 0) {
    char buf[4096];
    _getSha256(key, buf, idx);
    return stringToSHA256(string(buf));
}

void setSha256(const char* key, SHA256Hash& val, uint32_t idx = 0) {
    const char* buf = SHA256toString(val).c_str();
    _setSha256(key, buf, idx);
}
void setWallet(char* key, PublicWalletAddress& val, uint32_t idx = 0) {
    const char* buf = walletAddressToString(val).c_str();
    _setWallet(key, (char*) buf, idx);
}

// void setBytes(const char* key,  vector<uint8_t>& val, const uint64_t idx = 0);
// void setBigint(const char* key, Bigint& val, const uint64_t idx = 0);
// SHA256Hash getSha256(const char* key, uint64_t index = 0);
// Bigint getBigint(const char* key, uint64_t index = 0);
// PublicWalletAddress getWallet(const char* key, uint64_t index = 0);
// vector<uint8_t> getBytes(const char* key, uint64_t index = 0);

Block getBlock(const char* ptr) {
    BlockHeader blockH = blockHeaderFromBuffer(ptr);
    ptr += BLOCKHEADER_BUFFER_SIZE;
    vector<Transaction> transactions;
    for(int j = 0; j < blockH.numTransactions; j++) {
        TransactionInfo t = transactionInfoFromBuffer(ptr);
        ptr += transactionInfoBufferSize();
        Transaction tx(t);
        transactions.push_back(tx);
    }
    return Block(blockH, transactions);
}