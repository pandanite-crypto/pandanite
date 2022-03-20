#include "../core/crypto.hpp"
#include "../core/common.hpp"
#include <iostream>

using namespace std;

TEST(test_key_string_conversion) {
    std::pair<PublicKey,PrivateKey> keys = generateKeyPair();
    PublicKey p = keys.first;
    PrivateKey q = keys.second;
    int cmp = memcmp(stringToPublicKey(publicKeyToString(p)).data(), p.data(), 32);
    ASSERT_EQUAL(cmp, 0);
}

TEST(test_signature_string_conversion) {
    std::pair<PublicKey,PrivateKey> keys = generateKeyPair();
    PublicKey p = keys.first;
    PrivateKey q = keys.second;
    TransactionSignature t = signWithPrivateKey("FOOBAR", p, q);
    int cmp = memcmp(stringToSignature(signatureToString(t)).data(), t.data(), 64);
    ASSERT_EQUAL(cmp, 0);
}

TEST(test_signature_verifications) {
    std::pair<PublicKey,PrivateKey> keys = generateKeyPair();
    PublicKey p = keys.first;
    PrivateKey q = keys.second;
    TransactionSignature t = signWithPrivateKey("FOOBAR", p, q);
    bool status = checkSignature("FOOBAR", t, p);
    ASSERT_EQUAL(status, true);

    // check with wrong public key
    std::pair<PublicKey,PrivateKey> keys2 = generateKeyPair();
    PublicKey r = keys2.first;
    PrivateKey s = keys2.second;
    t = signWithPrivateKey("FOOBAR", r, s);
    status = checkSignature("FOOBAR", t, p);
    ASSERT_EQUAL(status, false);
}

TEST(total_work) {
    Bigint work = 0;
    work = addWork(work, 16);
    work = addWork(work, 16);
    work = addWork(work, 16);
    Bigint base = 2;
    Bigint mult = 3;
    Bigint expected = mult * base.pow(16);
    ASSERT_EQUAL(work, expected);
    ASSERT_EQUAL(work, 196608);
    work = addWork(work, 32);
    work = addWork(work, 28);
    work = addWork(work, 74);
    work = addWork(work, 174);
    Bigint b = 0;
    b+=expected;
    base = 2;
    b+=base.pow(32);
    base = 2;
    b+=base.pow(28);
    base = 2;
    b+=base.pow(74);
    base = 2;
    b+= base.pow(174);
    
    ASSERT_EQUAL(b, work);
    ASSERT_EQUAL(to_string(work), "23945242826029513411849172299242470459974281928572928");

}

TEST(mine_hash) {
    SHA256Hash hash = SHA256("Hello World");
    SHA256Hash answer = mineHash(hash, 16);
    SHA256Hash newHash = concatHashes(hash, answer);
    const char * a = (const char*) newHash.data();
    // check first 2 bytes (16 bits)
    ASSERT_TRUE(a[0] == 0 && a[1] == 0);
}

TEST(sha256_to_string) {
    SHA256Hash h = SHA256("FOOBAR");
    ASSERT_EQUAL(h, stringToSHA256(SHA256toString(h)));
}