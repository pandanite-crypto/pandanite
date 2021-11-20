
#include "crypto.hpp"
#include <iostream>
#include "../external/ed25519/ed25519.h" //https://github.com/orlp/ed25519
using namespace std;

SHA256Hash SHA256(const char* buffer, size_t len) {
    std::array<uint8_t, 32> ret;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, buffer, len);
    SHA256_Final(ret.data(), &sha256);
    return ret;
}

RIPEMD160Hash RIPEMD160(const char* buffer, size_t len) {
    std::array<uint8_t, 20> ret;
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

secp256k1_context* GLOBAL_CONTEXT = NULL;
secp256k1_context* getSecpContext() {
    if (GLOBAL_CONTEXT == NULL) {
        size_t context_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
        GLOBAL_CONTEXT = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    }
    return GLOBAL_CONTEXT;
}


PublicWalletAddress walletAddressFromPublicKey(PublicKey inputKey) {
    // Based on: https://en.bitcoin.it/wiki/Technical_background_of_version_1_Bitcoin_addresses
    SHA256Hash hash;
#ifdef SECP256K1
    secp256k1_context *ctx = getSecpContext();
    int res;
    size_t len;
    
    uint8_t pub[33];
    len = sizeof(pub);
    secp256k1_ec_pubkey_serialize(ctx, pub, &len, &inputKey, SECP256K1_EC_COMPRESSED);
    hash = SHA256((const char*)pub, len);
    
#else
    hash = SHA256((const char*)inputKey.data(), inputKey.size());
#endif
    
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
#ifdef SECP256K1
    secp256k1_context *ctx = getSecpContext();
    size_t context_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    int res;
    PrivateKey secret;
    for(size_t i = 0 ; i < secret.size(); i++) {
        secret[i] = rand()%256;
    }
    
    res = secp256k1_ec_seckey_verify(ctx, (uint8_t*) secret.data());
    if(!res) throw std::runtime_error("Private keygen failed");
    secp256k1_pubkey pubkey;
    res = secp256k1_ec_pubkey_create(ctx, &pubkey, (uint8_t*)secret.data());
    if(!res) throw std::runtime_error("Public keygen failed");
    
    return std::pair<PublicKey, PrivateKey>(pubkey, secret);
#else
    char buf[32];
    ed25519_create_seed((unsigned char*)buf);
    PublicKey pubKey;
    PrivateKey privKey;
    ed25519_create_keypair((unsigned char*)pubKey.data(), (unsigned char*)privKey.data(), (unsigned char*)buf);
    return std::pair<PublicKey, PrivateKey>(pubKey, privKey);
#endif
}



string walletAddressToString(PublicWalletAddress p) {
    return hexEncode((const char*)p.data(), p.size());
}

bool isFounderWalletPossible(PublicWalletAddress w) {
    SHA256Hash hash = SHA256((const char*)w.data(), w.size());
    int difficulty = FOUNDER_WALLET_BITS;
    bool found = checkLeadingZeroBits(hash, difficulty);
    return found;
}

PublicWalletAddress stringToWalletAddress(string s) {
    vector<uint8_t> bytes = hexDecode(s);
    PublicWalletAddress ret;
    std::move(bytes.begin(), bytes.begin() + ret.size(), ret.begin());
    return ret;
}

string privateKeyToString(PrivateKey p) {
    return hexEncode((const char*)p.data(), p.size());
}
PrivateKey stringToPrivateKey(string p) {
    vector<uint8_t> bytes = hexDecode(p);
    PrivateKey pKey;
    std::move(bytes.begin(), bytes.begin() + pKey.size(), pKey.begin());
    return pKey;
}

string publicKeyToString(PublicKey pubKey) {
#ifdef SECP256K1
    secp256k1_context *ctx = getSecpContext();
    size_t context_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    int8_t pub[33];
    size_t len = sizeof(pub);
    secp256k1_ec_pubkey_serialize(ctx, (unsigned char*)pub, &len, &pubKey, SECP256K1_EC_COMPRESSED);
    
    return hexEncode((const char*) pub,33);
#else
    return hexEncode((const char*) pubKey.data(),pubKey.size());
#endif
}
PublicKey stringToPublicKey(string p) {
#ifdef SECP256K1
    secp256k1_context *ctx = getSecpContext();
    int res;
    size_t len;
    size_t context_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    vector<uint8_t> data = hexDecode(p);
    secp256k1_pubkey pubkey;
    res = secp256k1_ec_pubkey_parse(ctx, &pubkey, &data[0], 33);
    if(!res) throw std::runtime_error("invalid key string");
    
    return pubkey;
#else
    vector<uint8_t> data = hexDecode(p);
    PublicKey pubKey;
    for(int i = 0; i < data.size(); i++) {
        pubKey[i] = data[i];
    }
    return pubKey;
#endif
}   

TransactionSignature signWithPrivateKey(string content, PublicKey pubKey, PrivateKey privKey) {
    return signWithPrivateKey(content.c_str(), content.length(), pubKey, privKey);
}

TransactionSignature signWithPrivateKey(const char* bytes, size_t len, PublicKey pubKey, PrivateKey privKey) {
#ifdef SECP256K1
    secp256k1_context *ctx = getSecpContext();
    int res;
    size_t context_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    SHA256Hash hash = SHA256(bytes, len);
    secp256k1_ecdsa_signature sig;
    res = secp256k1_ecdsa_sign(ctx, &sig, (uint8_t*)hash.data(), (uint8_t*)privKey.data(), NULL, NULL);
    if(!res) std::runtime_error("Can't sign");
    
    return sig;
#else
    TransactionSignature t;
    ed25519_sign(
        (unsigned char *)t.data(), 
        (const unsigned char *) bytes, len, 
        (const unsigned char *) pubKey.data(), 
        (const unsigned char *) privKey.data()
    );
    return t;
#endif
}

string signatureToString(TransactionSignature t) {
#ifdef SECP256K1
    secp256k1_context *ctx = getSecpContext();
    int res;
    size_t context_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    uint8_t der[72];
    size_t len = sizeof(der);
    res = secp256k1_ecdsa_signature_serialize_der(ctx, der, &len, &t);
    string derHex = hexEncode((const char*) der, len);
    string length = std::to_string(len);
    string ret = length + "|" + derHex;
    
    return ret;
#else
    return hexEncode((const char*) t.data(), t.size());
#endif
}
TransactionSignature stringToSignature(string t) {
#ifdef SECP256K1
    secp256k1_context *ctx = getSecpContext();
    std::string delimiter = "|";
    size_t idx = t.find(delimiter);
    int len = std::stoi(t.substr(0, idx));
    vector<uint8_t> bytes = hexDecode(t.substr(idx+1));

    size_t context_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    TransactionSignature ret;   
    secp256k1_ecdsa_signature_parse_der(ctx, &ret, bytes.data(),len);
    
    return ret;
#else
    vector<uint8_t> data = hexDecode(t);
    TransactionSignature decoded;
    for(int i = 0; i < data.size(); i++) {
        decoded[i] = data[i];
    }
    return decoded;
#endif
}

bool checkSignature(string content, TransactionSignature signature, PublicKey signingKey) {
    return checkSignature(content.c_str(), content.length(), signature, signingKey);
}

bool checkSignature(const char* bytes, size_t len, TransactionSignature signature, PublicKey signingKey) {
#ifdef SECP256K1
     secp256k1_context *ctx = getSecpContext();
    int res;
    size_t context_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    SHA256Hash hash = SHA256(bytes, len);
    res = secp256k1_ecdsa_verify(ctx, &signature, (unsigned char*) hash.data(), &signingKey);
    
    return res == 1;
#else
    int status = ed25519_verify((const unsigned char *) signature.data(),
                   (const unsigned char *) bytes, len,
                   (const unsigned char *) signingKey.data());
    return status == 1;
#endif
}

SHA256Hash concatHashes(SHA256Hash a, SHA256Hash b) {
    vector<uint8_t> concat;
    for(size_t i = 0; i < a.size(); i++) {
        concat.push_back(a[i]);
    }
    for(size_t i = 0; i < b.size(); i++) {
        concat.push_back(b[i]);
    }
    SHA256Hash fullHash  = SHA256((const char*)concat.data(), concat.size());
    return fullHash;
}

bool checkLeadingZeroBits(SHA256Hash hash, unsigned int challengeSize) {
    int bytes = challengeSize / 8;
    const uint8_t * a = hash.data();
    if (bytes == 0) {
        return a[0]>>(8-challengeSize) == 0;
    } else {
        for (int i = 0; i < bytes; i++) {
            if (a[i] != 0) return false;
        }
        int remainingBits = challengeSize - 8*bytes;
        if (remainingBits > 0) return a[bytes + 1]>>(8-remainingBits) == 0;
        else return true;
    }
    return false;
}


bool verifyHash(SHA256Hash target, SHA256Hash nonce, unsigned char challengeSize) {
    SHA256Hash fullHash  = concatHashes(target, nonce);
    return checkLeadingZeroBits(fullHash, challengeSize);
}

SHA256Hash mineHash(SHA256Hash targetHash, unsigned char challengeSize) {
    while(true) {
        SHA256Hash solution;
        for(size_t i = 0; i < solution.size(); i++) {
            solution[i] = rand()%256;
        }
        bool found = verifyHash(targetHash, solution, challengeSize);
        if (found) {
            return solution;
        }
    }
}