#include "user.hpp"
#include "helpers.hpp"
#include "crypto.hpp"
#include <string>
#include <iostream>
using namespace std;

User::User() {
    std::pair<PublicKey,PrivateKey> keyPair = generateKeyPair();
    this->publicKey = keyPair.first;
    this->privateKey = keyPair.second;
}

User::User(json u) {
    this->publicKey = stringToPublicKey(u["publicKey"]);
    this->privateKey = stringToPrivateKey(u["privateKey"]);
}

json User::toJson() {
    json result;
    result["publicKey"] = publicKeyToString(this->publicKey);
    result["privateKey"] = privateKeyToString(this->privateKey);
    return result;
}

PublicWalletAddress User::getAddress() {
    return walletAddressFromPublicKey(this->publicKey);
}

PublicKey User::getPublicKey() {
    return this->publicKey;
}

PrivateKey User::getPrivateKey() {
    return this->privateKey;
}

Transaction User::mine() {
    return Transaction(this->getAddress(), BMB(50));
}

Transaction User::send(User& to, TransactionAmount amount) {
    PublicWalletAddress fromWallet = this->getAddress();
    PublicWalletAddress toWallet = to.getAddress();
    Transaction t = Transaction(fromWallet, toWallet, amount, this->publicKey);
    this->signTransaction(t);
    return t;
}

void User::signTransaction(Transaction & t) {
    t.sign(this->publicKey, this->privateKey);
}