#pragma once
#include "../external/json.hpp"
#include "../external/bigint/bigint.h"
#include "openssl/sha.h"
 #include "openssl/ripemd.h"
#include <map>
#include <vector>
#include <string>
#include <array>
using namespace Dodecahedron;
using namespace std;
using namespace nlohmann;



typedef std::array<uint8_t, 25> PublicWalletAddress;
typedef uint64_t TransactionAmount;

typedef std::array<uint8_t,32> PublicKey;
typedef std::array<uint8_t,64> PrivateKey;
typedef std::array<uint8_t,64> TransactionSignature;


typedef map<PublicWalletAddress,TransactionAmount> LedgerState;
typedef std::array<uint8_t, 32> SHA256Hash;
typedef std::array<uint8_t, 20> RIPEMD160Hash;

#define NULL_SHA256_HASH SHA256Hash({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0})
#define NULL_KEY SHA256Hash({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0})
#define NULL_ADDRESS PublicWalletAddress({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0})