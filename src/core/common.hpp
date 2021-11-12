#pragma once
#include "nlohmann/json.hpp"
#include <secp256k1.h>
#include <secp256k1_preallocated.h>
#include "openssl/sha.h"
 #include "openssl/ripemd.h"
#include <map>
#include <vector>
#include <string>
#include <array>
using namespace std;
using namespace nlohmann;


typedef std::array<uint8_t, 25> PublicWalletAddress;
typedef long TransactionAmount;
typedef secp256k1_ecdsa_signature TransactionSignature;
typedef secp256k1_pubkey PublicKey;
typedef std::array<uint8_t, 32> PrivateKey;
typedef map<PublicWalletAddress,TransactionAmount> LedgerState;
typedef std::array<uint8_t, 32> SHA256Hash;
typedef std::array<uint8_t, 20> RIPEMD160Hash;

#define NULL_SHA256_HASH SHA256Hash({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0})