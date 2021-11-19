#pragma once
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <secp256k1.h>
#include <secp256k1_preallocated.h>
#include "openssl/sha.h"
#include "openssl/ripemd.h"
#include "common.hpp"
#include "constants.hpp"
using namespace std;

SHA256Hash SHA256(const char* buffer, size_t len);
SHA256Hash SHA256(string str);
RIPEMD160Hash  RIPEMD(const char* buffer, size_t len);
SHA256Hash stringToSHA256(string hex);
string SHA256toString(SHA256Hash h);
std::vector<uint8_t> hexDecode(const string& hex);
string hexEncode(const char* buffer, size_t len);
SHA256Hash concatHashes(SHA256Hash a, SHA256Hash b);
bool checkLeadingZeroBits(SHA256Hash hash, unsigned int challengeSize);

bool isFounderWalletPossible(PublicWalletAddress w);
PublicWalletAddress walletAddressFromPublicKey(PublicKey inputKey);
string walletAddressToString(PublicWalletAddress p);
PublicWalletAddress stringToWalletAddress(string s);
std::pair<PublicKey,PrivateKey> generateKeyPair();
string publicKeyToString(PublicKey p);
PublicKey stringToPublicKey(string p);
string privateKeyToString(PrivateKey p);
PrivateKey stringToPrivateKey(string p);

string signatureToString(TransactionSignature t);
TransactionSignature stringToSignature(string t);

TransactionSignature signWithPrivateKey(string content, PublicKey pubKey, PrivateKey privKey);
TransactionSignature signWithPrivateKey(const char* bytes, size_t len, PublicKey pubKey, PrivateKey privKey);
bool checkSignature(string content, TransactionSignature signature, PublicKey signingKey);
bool checkSignature(const char* bytes, size_t len, TransactionSignature signature, PublicKey signingKey);
SHA256Hash mineHash(SHA256Hash targetHash, unsigned char challengeSize);
bool verifyHash(SHA256Hash target, SHA256Hash nonce, unsigned char challengeSize);


