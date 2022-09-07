
#include "crypto.hpp"
#include "helpers.hpp"
#include "logger.hpp"
#include "constants.hpp"
#include <iostream>
#include <atomic>
#include <thread>
#include <random>
#include "../external/ed25519/ed25519.h" //https://github.com/orlp/ed25519
#include "../external/pufferfish/pufferfish.h" //https://github.com/epixoip/pufferfish
#include "../external/sha256/sha2.hpp" 
#include "../server/pufferfish_cache.hpp"
using namespace std;


PufferfishCache * pufferfishCache = NULL;
std::mutex pufferfishCacheLock;

SHA256Hash PUFFERFISH(const char* buffer, size_t len, bool useCache) {
    SHA256Hash inputHash;
    if (useCache) {
        std::unique_lock<std::mutex> ul(pufferfishCacheLock);
        memcpy(inputHash.data(), buffer, 32);
        if (!pufferfishCache) {
            pufferfishCache = new PufferfishCache();
            pufferfishCache->init(PUFFERFISH_CACHE_FILE_PATH);
        }
        SHA256Hash h;
        try {
            h = pufferfishCache->getHash(inputHash);
            return h;
        } catch(...) {}
    }
    char hash[PF_HASHSPACE];
    memset(hash, 0, PF_HASHSPACE);
    int ret = 0;
    if ((ret = pf_newhash((const void*) buffer, len, 0, 8, hash)) != 0) {
        Logger::logStatus("PUFFERFISH failed to compute hash");
    }
    size_t sz = PF_HASHSPACE;

    auto finalHash =  SHA256(hash, sz);

    if (useCache) {
        std::unique_lock<std::mutex> ul(pufferfishCacheLock);
        pufferfishCache->setHash(inputHash, finalHash);
    }
    return finalHash;
}

SHA256Hash SHA256(const char* buffer, size_t len, bool usePufferFish, bool useCache) {
    if (usePufferFish) return PUFFERFISH(buffer, len, useCache);
    std::array<uint8_t, 32> ret;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, buffer, len);
    SHA256_Final(ret.data(), &sha256);
    return ret;
}


std::array<uint8_t, 32> SHA256Fast(const char* buffer, size_t len) {
    std::array<uint8_t, 32> ret;
    sha256_ctx sha256;
    sha256_init(&sha256);
    sha256_update(&sha256, (const unsigned char*) buffer, len);
    sha256_final(&sha256, ret.data());
    return ret;
}

RIPEMD160Hash RIPEMD160(const char* buffer, size_t len) {
    RIPEMD160Hash ret;
    RIPEMD160_CTX ripemd;
    RIPEMD160_Init(&ripemd);
    RIPEMD160_Update(&ripemd, buffer, len);
    RIPEMD160_Final((unsigned char*)ret.data(), &ripemd);
    return ret;
}

SHA256Hash SHA256(string str) {
    return SHA256(str.c_str(), str.length());
}

std::vector<uint8_t> hexDecode(const string& hex) {
  vector<uint8_t> bytes;
  for (size_t i = 0; i < hex.length(); i += 2) {
    string byteString = hex.substr(i, 2);
    char byte = (char) strtol(byteString.c_str(), NULL, 16);
    bytes.push_back(byte);
  }
  return std::move(bytes);
}

string hexEncode(const char* buffer, size_t len) {
    static const char* const lut = "0123456789ABCDEF";
    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = buffer[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

SHA256Hash stringToSHA256(string hex) {
    vector<uint8_t> bytes = hexDecode(hex);
    SHA256Hash sha;
    std::move(bytes.begin(), bytes.begin()+sha.size(), sha.begin());
    return sha;
}

string SHA256toString(SHA256Hash h) {
    return hexEncode((const char*)h.data(), h.size());
}

PublicWalletAddress walletAddressFromPublicKey(PublicKey inputKey) {
    // Based on: https://en.bitcoin.it/wiki/Technical_background_of_version_1_Bitcoin_addresses
    SHA256Hash hash;
    hash = SHA256((const char*)inputKey.data(), inputKey.size());
    RIPEMD160Hash hash2 = RIPEMD160((const char*)hash.data(), hash.size());
    SHA256Hash hash3 = SHA256((const char*)hash2.data(),hash2.size());
    SHA256Hash hash4 = SHA256((const char*)hash3.data(),hash3.size());
    uint8_t checksum = hash4[0];
    PublicWalletAddress address;
    address[0] = 0;
    for(int i = 1; i <=20; i++) {
        address[i] = hash2[i-1];
    }
    address[21] = hash4[0];
    address[22] = hash4[1];
    address[23] = hash4[2];
    address[24] = hash4[3];
    
    return address;

}

std::pair<PublicKey,PrivateKey> generateKeyPair() {
    char buf[32];
    ed25519_create_seed((unsigned char*)buf);
    PublicKey pubKey;
    PrivateKey privKey;
    ed25519_create_keypair((unsigned char*)pubKey.data(), (unsigned char*)privKey.data(), (unsigned char*)buf);
    return std::pair<PublicKey, PrivateKey>(pubKey, privKey);
}

string walletAddressToString(PublicWalletAddress p) {
    return hexEncode((const char*)p.data(), p.size());
}

PublicWalletAddress stringToWalletAddress(string s) {
    if (s.size() != 50) throw std::runtime_error("Invalid wallet address string");
    vector<uint8_t> bytes = hexDecode(s);
    PublicWalletAddress ret;
    std::move(bytes.begin(), bytes.begin() + ret.size(), ret.begin());
    return ret;
}

string privateKeyToString(PrivateKey p) {
    return hexEncode((const char*)p.data(), p.size());
}
PrivateKey stringToPrivateKey(string p) {
    if (p.size() != 128) throw std::runtime_error("Invalid private key string");
    vector<uint8_t> bytes = hexDecode(p);
    PrivateKey pKey;
    std::move(bytes.begin(), bytes.begin() + pKey.size(), pKey.begin());
    return pKey;
}

string publicKeyToString(PublicKey pubKey) {
    return hexEncode((const char*) pubKey.data(),pubKey.size());
}
PublicKey stringToPublicKey(string p) {
    if (p.size() != 64) throw std::runtime_error("Invalid public key string");
    vector<uint8_t> data = hexDecode(p);
    PublicKey pubKey;
    for(int i = 0; i < data.size(); i++) {
        pubKey[i] = data[i];
    }
    return pubKey;
}   

TransactionSignature signWithPrivateKey(string content, PublicKey pubKey, PrivateKey privKey) {
    return signWithPrivateKey(content.c_str(), content.length(), pubKey, privKey);
}

TransactionSignature signWithPrivateKey(const char* bytes, size_t len, PublicKey pubKey, PrivateKey privKey) {
    TransactionSignature t;
    ed25519_sign(
        (unsigned char *)t.data(), 
        (const unsigned char *) bytes, len, 
        (const unsigned char *) pubKey.data(), 
        (const unsigned char *) privKey.data()
    );
    return t;
}

string signatureToString(TransactionSignature t) {
    return hexEncode((const char*) t.data(), t.size());
}
TransactionSignature stringToSignature(string t) {
    vector<uint8_t> data = hexDecode(t);
    TransactionSignature decoded;
    for(int i = 0; i < data.size(); i++) {
        decoded[i] = data[i];
    }
    return decoded;
}

bool checkSignature(string content, TransactionSignature signature, PublicKey signingKey) {
    return checkSignature(content.c_str(), content.length(), signature, signingKey);
}

bool checkSignature(const char* bytes, size_t len, TransactionSignature signature, PublicKey signingKey) {
    int status = ed25519_verify((const unsigned char *) signature.data(),
                   (const unsigned char *) bytes, len,
                   (const unsigned char *) signingKey.data());
    return status == 1;
}

SHA256Hash concatHashes(SHA256Hash& a, SHA256Hash& b, bool usePufferFish, bool useCache) {
    char data[64];
    memcpy(data, (char*)a.data(), 32);
    memcpy(&data[32], (char*)b.data(), 32);
    SHA256Hash fullHash  = SHA256((const char*)data, 64, usePufferFish, useCache);
    return fullHash;
}

bool checkLeadingZeroBits(SHA256Hash& hash, uint8_t challengeSize) {
    int bytes = challengeSize / 8; 
    const uint8_t * a = hash.data();
    for (int i = 0; i < bytes; i++) {
        if (a[i] != 0) return false;
    }
    int remainingBits = challengeSize - 8*bytes;
    if (remainingBits > 0) return a[bytes]>>(8-remainingBits) == 0;
    else return true;
}

Bigint addWork(Bigint previousWork, uint32_t challengeSize) {
    Bigint base = 2;
    previousWork+= base.pow((int) challengeSize);
    return previousWork;
}

bool verifyHash(SHA256Hash& target, SHA256Hash& nonce, uint8_t challengeSize, bool usePufferFish, bool useCache) {
    SHA256Hash fullHash  = concatHashes(target, nonce, usePufferFish, useCache);
    return checkLeadingZeroBits(fullHash, challengeSize);
}


SHA256Hash mineHash(SHA256Hash target, unsigned char challengeSize, bool usePufferFish) {

    int hashes = 0;
    int threadId = 0;
    int64_t st = getTimeMilliseconds();

    // By @Shifu!
    vector<uint8_t> concat;
    SHA256Hash solution;
    std::random_device t_rand;
    uint32_t i = 0;

    concat.resize(2 * 32);
    for (size_t i = 0; i < 32; i++) concat[i] = target[i];
    // fill with random data for privacy (one cannot guess number of tries later)
    for (size_t i = 32; i < 64; ++i) concat[i] = t_rand() % 256;

    while(true) {

        if (hashes++ > 1024) {
            double hps = (double)hashes / ((double)(getTimeMilliseconds() - st) / 1000);
            Logger::logStatus("[thread" + std::to_string(threadId) + "] mining at " + std::to_string(hps) + "h/sec");
            hashes = 0;
            st = getTimeMilliseconds();
        }

        ++i;
        *reinterpret_cast<uint64_t*>(concat.data() + 32) += 1;
        memcpy(solution.data(), concat.data() + 32, 32);
        SHA256Hash fullHash = concatHashes(target, solution, usePufferFish);

        bool found = checkLeadingZeroBits(fullHash, challengeSize);

        if (found) {
            break;
        }
    };

    return solution;
}
