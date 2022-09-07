#include "transaction.hpp"
#include "helpers.hpp"
#include "block.hpp"
#include "openssl/sha.h"
#include <sstream>
#include <iostream>
 #include <cstring>
#include <ctime>
using namespace std;


TransactionInfo transactionInfoFromBuffer(const char* buffer) {
    TransactionInfo t;
    readNetworkNBytes(buffer, t.signature, 64);
    readNetworkNBytes(buffer, t.signingKey, 32);
    t.timestamp = readNetworkUint64(buffer);
    t.to = readNetworkPublicWalletAddress(buffer);
    t.amount = readNetworkUint64(buffer);
    t.fee = readNetworkUint64(buffer);
    t.isTransactionFee = readNetworkUint32(buffer) > 0;

    if (!t.isTransactionFee) {
        PublicKey pub;
        memcpy(pub.data(), t.signingKey, 32);
        t.from = walletAddressFromPublicKey(pub);
    } else {
        t.from = NULL_ADDRESS;
    }

    return t;
}

void transactionInfoToBuffer(TransactionInfo& t, char* buffer) {
    writeNetworkNBytes(buffer, t.signature, 64);
    writeNetworkNBytes(buffer, t.signingKey, 32);
    writeNetworkUint64(buffer, t.timestamp);
    writeNetworkPublicWalletAddress(buffer, t.to);
    writeNetworkUint64(buffer, t.amount);
    writeNetworkUint64(buffer, t.fee);
    uint32_t flag = 0;
    if (t.isTransactionFee) flag = 1;
    writeNetworkUint32(buffer, flag);
}

Transaction::Transaction(PublicWalletAddress from, PublicWalletAddress to, TransactionAmount amount, PublicKey signingKey, TransactionAmount fee) {
    this->from = from;
    this->to = to;
    this->amount = amount;
    this->isTransactionFee = false;
    this->timestamp = std::time(0);
    this->fee = fee;
    this->signingKey = signingKey;
}

Transaction::Transaction(PublicWalletAddress from, PublicWalletAddress to, TransactionAmount amount, PublicKey signingKey, TransactionAmount fee, uint64_t timestamp) {
    this->from = from;
    this->to = to;
    this->amount = amount;
    this->isTransactionFee = false;
    this->timestamp = timestamp;
    this->fee = fee;
    this->signingKey = signingKey;
}

Transaction::Transaction() {

}

Transaction::Transaction(const TransactionInfo& t) {
    this->to = t.to;
    if (!t.isTransactionFee) this->from = t.from;
    memcpy((void*)this->signature.data(), (void*)t.signature, 64);
    memcpy((void*)this->signingKey.data(), (void*)t.signingKey, 32);
    this->amount = t.amount;
    this->isTransactionFee = t.isTransactionFee;
    this->timestamp = t.timestamp;
    this->fee = t.fee;
}
TransactionInfo Transaction::serialize() {
    TransactionInfo t;
    memcpy((void*)t.signature, (void*)this->signature.data(), 64);
    memcpy((void*)t.signingKey, (void*)this->signingKey.data(), 32);
    t.timestamp = this->timestamp;
    t.to = this->to;
    t.from = this->from;
    t.amount = this->amount;
    t.fee = this->fee;
    t.isTransactionFee = this->isTransactionFee;
    return t;
}


Transaction::Transaction(const Transaction & t) {
    this->to = t.to;
    this->from = t.from;
    this->signature = t.signature;
    this->amount = t.amount;
    this->isTransactionFee = t.isTransactionFee;
    this->timestamp = t.timestamp;
    this->fee = t.fee;
    this->signingKey = t.signingKey;
}

Transaction::Transaction(PublicWalletAddress to, TransactionAmount fee) {
    this->to = to;
    this->amount = fee;
    this->isTransactionFee = true;
    this->timestamp = getCurrentTime();
    this->fee = 0;
}

Transaction::Transaction(json data) {
    PublicWalletAddress to;
    this->timestamp = stringToUint64(data["timestamp"]);
    this->to = stringToWalletAddress(data["to"]);
    this->fee = data["fee"];
    if(data["from"] == "") {        
        this->amount = data["amount"];
        this->isTransactionFee = true;
    } else {
        this->from = stringToWalletAddress(data["from"]);
        this->signature = stringToSignature(data["signature"]);
        this->amount = data["amount"];
        this->isTransactionFee = false;
        this->signingKey = stringToPublicKey(data["signingKey"]);
    }
}

void Transaction::setTransactionFee(TransactionAmount amount) {
    this->fee = amount;
}
TransactionAmount Transaction::getTransactionFee() const {
    return this->fee;
}


json Transaction::toJson() {
    json result;
    result["to"] = walletAddressToString(this->toWallet());
    result["amount"] = this->amount;
    result["timestamp"] = uint64ToString(this->timestamp);
    result["fee"] = this->fee;
    if (!this->isTransactionFee) {
        result["txid"] = SHA256toString(this->hashContents());
        result["from"] = walletAddressToString(this->fromWallet());
        result["signingKey"] = publicKeyToString(this->signingKey);
        result["signature"] = signatureToString(this->signature);
    } else {
        result["txid"] = SHA256toString(this->hashContents());
        result["from"] = "";
    }
    
    return result;
}


bool Transaction::isFee() const {
    return this->isTransactionFee;
}

void Transaction::setTimestamp(uint64_t t) {
    this->timestamp = t;
}

uint64_t Transaction::getTimestamp() {
    return this->timestamp;
}

TransactionSignature Transaction::getSignature() const {
    return this->signature;
}

void Transaction::setAmount(TransactionAmount amt) {
    this->amount = amt;
}

bool Transaction::signatureValid() const {
    if (this->isFee()) return true;
    SHA256Hash hash = this->hashContents();
    return checkSignature((const char*)hash.data(), hash.size(), this->signature, this->signingKey);

}

PublicKey Transaction::getSigningKey() {
    return this->signingKey;
}


SHA256Hash Transaction::getHash() const {
    SHA256Hash ret;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256Hash contentHash = this->hashContents();
    SHA256_Update(&sha256, (unsigned char*)contentHash.data(), contentHash.size());
    if (!this->isTransactionFee) {
        SHA256_Update(&sha256, (unsigned char*)this->signature.data(), this->signature.size());
    }
    SHA256_Final(ret.data(), &sha256);
    return ret;
}

PublicWalletAddress Transaction::fromWallet() const {
    return this->from;
}
PublicWalletAddress Transaction::toWallet() const {
    return this->to;
}

TransactionAmount Transaction::getAmount() const {
    return this->amount;
}

TransactionAmount Transaction::getFee() const {
    return this->fee;
}

SHA256Hash Transaction::hashContents() const {
    SHA256Hash ret;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    PublicWalletAddress wallet = this->toWallet();
    SHA256_Update(&sha256, (unsigned char*)wallet.data(), wallet.size());
    if (!this->isTransactionFee) {
        wallet = this->fromWallet();
        SHA256_Update(&sha256, (unsigned char*)wallet.data(), wallet.size());
    }
    SHA256_Update(&sha256, (unsigned char*)&this->fee, sizeof(TransactionAmount));
    SHA256_Update(&sha256, (unsigned char*)&this->amount, sizeof(TransactionAmount));
    SHA256_Update(&sha256, (unsigned char*)&this->timestamp, sizeof(uint64_t));
    SHA256_Final(ret.data(), &sha256);
    return ret;
}

void Transaction::sign(PublicKey pubKey, PrivateKey signingKey) {
    SHA256Hash hash = this->hashContents();
    TransactionSignature signature = signWithPrivateKey((const char*)hash.data(), hash.size(), pubKey, signingKey);
    this->signature = signature;
}

bool operator<(const Transaction& a, const Transaction& b) {
    return a.signature < b.signature;
}

bool operator==(const Transaction& a, const Transaction& b) {
    if(a.timestamp != b.timestamp) return false;
    if(a.toWallet() != b.toWallet()) return false;
    if(a.getTransactionFee() != b.getTransactionFee()) return false;
    if( a.amount != b.amount) return false;
    if( a.isTransactionFee != b.isTransactionFee) return false;
    if (!a.isTransactionFee) {
        if( a.fromWallet() != b.fromWallet()) return false;
        if(signatureToString(a.signature) != signatureToString(b.signature)) return false;
        if (publicKeyToString(a.signingKey) != publicKeyToString(b.signingKey)) return false;
    }
    return true;
}