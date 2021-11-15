#include <iostream>
#include <mutex>
#include <thread>
#include <map>
#include <functional>
#include <secp256k1.h>
#include <secp256k1_preallocated.h>
#include "../core/crypto.hpp"
using namespace std;


void key_search() {

    secp256k1_context *ctx = NULL;
    size_t context_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    time_t last = std::time(0);
    long i = 0;
    cout<<"Starting search for keys"<<endl;
    while(true) {
        int res;
        PrivateKey privateKey;
        for(size_t i = 0 ; i < privateKey.size(); i++) {
            privateKey[i] = rand()%256;
        }
        
        res = secp256k1_ec_seckey_verify(ctx, (uint8_t*) privateKey.data());
        if(!res) continue;
        secp256k1_pubkey publicKey;
        res = secp256k1_ec_pubkey_create(ctx, &publicKey, (uint8_t*)privateKey.data());
        if(!res) continue;

        time_t curr = std::time(0);
        SHA256Hash hash = SHA256(publicKeyToString(publicKey));
        
        int difficulty = 20;
        bool found = checkLeadingZeroBits(hash, difficulty);
        if (found) {
            cout<<"Found wallet"<<endl;
            cout<<"pubKey:"<<publicKeyToString(publicKey)<<endl;
            cout<<"privateKey:"<<privateKeyToString(privateKey)<<endl;
            cout<<"Time:"<<(curr-last)<<endl;
            last = curr;
        }
        i++;
        if (i%100000 == 0) {
            cout<<"searched "<<i<<" keys so far"<<endl;
        }
    } 
    secp256k1_context_destroy(ctx);
}

int main() {

    std::thread key_search_thread(key_search);

    key_search_thread.join();
}