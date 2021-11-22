#pragma once
#include "nlohmann/json.hpp"
#include "openssl/sha.h"
 #include "openssl/ripemd.h"
#include <map>
#include <vector>
#include <string>
#include <array>

#ifdef SECP256K1
#include <secp256k1.h>
#include <secp256k1_preallocated.h>
#endif

using namespace std;
using namespace nlohmann;



typedef std::array<uint8_t, 25> PublicWalletAddress;
typedef long TransactionAmount;

#ifdef SECP256K1
typedef secp256k1_pubkey PublicKey;
typedef std::array<uint8_t, 32> PrivateKey;
typedef secp256k1_ecdsa_signature TransactionSignature;
#else
typedef std::array<uint8_t,32> PublicKey;
typedef std::array<uint8_t,64> PrivateKey;
typedef std::array<uint8_t,64> TransactionSignature;
#endif


typedef map<PublicWalletAddress,TransactionAmount> LedgerState;
typedef std::array<uint8_t, 32> SHA256Hash;
typedef std::array<uint8_t, 20> RIPEMD160Hash;

#define NULL_SHA256_HASH SHA256Hash({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0})
#define NULL_ADDRESS PublicWalletAddress({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0})