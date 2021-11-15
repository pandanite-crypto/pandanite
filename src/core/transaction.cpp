#include "transaction.hpp"
#include "helpers.hpp"
#include "block.hpp"
#include <sstream>
#include <iostream>
 #include <cstring>
#include <ctime>
using namespace std;

Transaction::Transaction(PublicWalletAddress from, PublicWalletAddress to, TransactionAmount amount, int blockId, PublicKey signingKey, TransactionAmount fee) {
    this->from = from;
    this->to = to;
    this->amount = amount;
    this->isTransactionFee = false;
    this->blockId = blockId;
    this->nonce = randomString(TRANSACTION_NONCE_SIZE);
    this->timestamp = std::time(0);
    this->fee = fee;
    this->signingKey = signingKey;
}

Transaction::Transaction() {

}

Transaction::Transaction(const TransactionInfo& t) {
    this->to = t.to;
    if (!t.isTransactionFee) this->from = t.from;
    memcpy((void*)this->signature.data, (void*)t.signature, 64);
    this->nonce = string((char*)t.nonce, TRANSACTION_NONCE_SIZE);
    memcpy((void*)this->signingKey.data, (void*)t.signingKey, 64);
    this->amount = t.amount;
    this->isTransactionFee = t.isTransactionFee;
    this->blockId = t.blockId;
    this->timestamp = t.timestamp;
    this->fee = t.fee;
    if (t.hasMiner) {
        this->setMinerWallet(t.miner);
    }
}
TransactionInfo Transaction::serialize() {
    TransactionInfo t;
    t.blockId = this->blockId;
    memcpy((void*)t.signature, (void*)this->signature.data, 64);
    memcpy((void*)t.signingKey, (void*)this->signingKey.data, 64);
    memcpy((char*)t.nonce, (char*)this->nonce.c_str(), TRANSACTION_NONCE_SIZE);
    t.timestamp = this->timestamp;
    t.to = this->to;
    t.from = this->from;
    t.amount = this->amount;
    t.fee = this->fee;
    t.isTransactionFee = this->isTransactionFee;
    t.hasMiner = this->hasMiner();
    if (this->hasMiner()) {
        t.miner = this->miner.value();
    }
    return t;
}


Transaction::Transaction(const Transaction & t) {
    this->to = t.to;
    this->from = t.from;
    this->signature = t.signature;
    this->amount = t.amount;
    this->isTransactionFee = t.isTransactionFee;
    this->blockId = t.blockId;
    this->nonce = t.nonce;
    this->timestamp = t.timestamp;
    this->fee = t.fee;
    this->signingKey = t.signingKey;
    this->miner = t.miner;
}

Transaction::Transaction(PublicWalletAddress to, int blockId) {
    this->to = to;
    this->amount = MINING_FEE; // TODO make this a function
    this->isTransactionFee = true;
    this->blockId = blockId;
    this->nonce = randomString(TRANSACTION_NONCE_SIZE);
    this->timestamp = getCurrentTime();
    this->fee = 0;
}

Transaction::Transaction(json data) {
    PublicWalletAddress to;
    this->timestamp = stringToTime(data["timestamp"]);
    this->to = stringToWalletAddress(data["to"]);
    this->blockId = data["id"];
    this->nonce = data["nonce"];
    this->fee = data["fee"];
    if(data["miner"]!="") this->miner = stringToWalletAddress(data["miner"]);
    if(data["from"] == "") {        
        this->amount = MINING_FEE; // TODO: Mining fee should come from function
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

bool Transaction::hasMiner() const {
    return this->miner.has_value();
}

json Transaction::toJson() {
    json result;
    result["to"] = walletAddressToString(this->toWallet());
    result["amount"] = this->amount;
    result["timestamp"] = timeToString(this->timestamp);
    result["id"] = this->blockId;
    result["nonce"] = this->nonce;
    if (this->miner.has_value()) {
        result["miner"] = walletAddressToString(this->miner.value());
    } else {
        result["miner"] = "";
    }
    result["fee"] = this->fee;
    if (!this->isTransactionFee) {
        result["from"] = walletAddressToString(this->fromWallet());
        result["signingKey"] = publicKeyToString(this->signingKey);
        result["signature"] = signatureToString(this->signature);
    } else {
        result["from"] = "";
    }
    
    return result;
}

void Transaction::setMinerWallet(PublicWalletAddress addr) {
    this->miner = addr;
}
PublicWalletAddress Transaction::getMinerWallet() const {
    if (!this->miner.has_value()) throw std::runtime_error("Miner does not exist on transaction");
    return this->miner.value();
}

bool Transaction::isFee() const {
    return this->isTransactionFee;
}

void Transaction::setTimestamp(time_t t) {
    this->timestamp = t;
}

time_t Transaction::getTimestamp() {
    return this->timestamp;
}


void Transaction::setNonce(string n) {
    this->nonce = n;
}

string Transaction::getNonce() const {
    return this->nonce;
}

void Transaction::setBlockId(int id) {
    this->blockId = id;
}

int Transaction::getBlockId() const {
    return this->blockId;
}

TransactionSignature Transaction::getSignature() const {
    return this->signature;
}

void Transaction::setAmount(TransactionAmount amt) {
    this->amount = amt;
}

bool Transaction::signatureValid() const {
    if (this->isFee()) return true;
    return checkSignature(this->toString(), this->signature, this->signingKey);

}

SHA256Hash Transaction::getHash() const {
    string fullStr = this->toString() + "|";
    if (!this->isTransactionFee) fullStr += signatureToString(this->signature);
    return SHA256(fullStr);
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

string Transaction::toString() const {
    stringstream s;
    s<<walletAddressToString(this->toWallet())<<"|";
    if (!this->isTransactionFee) s<<walletAddressToString(this->fromWallet())<<"|";
    s<<"|"<<this->getTransactionFee()<<"|";
    s<<this->amount <<"|" <<this->blockId <<"|";
    s<<this->nonce<<"|"<<timeToString(this->timestamp)<<"|";
    return s.str();
}

void Transaction::sign(PrivateKey signingKey) {
    TransactionSignature signature = signWithPrivateKey(this->toString(), signingKey);
    this->signature = signature;
}

bool operator==(const Transaction& a, const Transaction& b) {
    if (a.blockId != b.blockId) return false;
    if( a.nonce != b.nonce) return false;
    if(a.timestamp != b.timestamp) return false;
    if(a.toWallet() != b.toWallet()) return false;
    if(a.hasMiner() != b.hasMiner()) return false;
    if(a.hasMiner() && a.getMinerWallet() != b.getMinerWallet()) return false;
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