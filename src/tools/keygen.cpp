#include <iostream>
#include <mutex>
#include <thread>
#include <map>
#include <functional>
#include <fstream>
#ifdef SECP256K1
#include <secp256k1.h>
#include <secp256k1_preallocated.h>
#endif
#include "../core/crypto.hpp"
using namespace std;


void key_search() {
#ifdef SECP256K1
    secp256k1_context *ctx = NULL;
    size_t context_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
#endif
    time_t last = std::time(0);
    long i = 0;
    cout<<"=====GENERATE WALLET===="<<endl;
    ofstream myfile;
    cout<<"Enter output file name"<<endl;
    string filename;
    cin>>filename;
    myfile.open(filename);
    while(true) {
#ifdef SECP256K1
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
#else
        std::pair<PublicKey,PrivateKey> pair = generateKeyPair();
        PublicKey publicKey = pair.first;
        PrivateKey privateKey = pair.second;
#endif
        time_t curr = std::time(0);
        PublicWalletAddress w = walletAddressFromPublicKey(publicKey);
        bool found = isFounderWalletPossible(w);
        if (found) {
            string wallet = walletAddressToString(walletAddressFromPublicKey(publicKey));
            string pubKey = publicKeyToString(publicKey);
            string privKey = privateKeyToString(privateKey);
            cout<<"Found wallet!"<<endl;
            cout<<"========================"<<endl;
            cout<<"Wallet Address:"<<wallet<<endl;
            cout<<"Public Key    :"<<pubKey<<endl;
            cout<<"Private Key   :"<<privKey<<endl;
            cout<<"Time (seconds):"<<(curr-last)<<endl;
            cout<<"========================"<<endl;
            cout<<"Output written to "<<filename<<endl;
            myfile<<"wallet:"<<wallet<<endl;
            myfile<<"publickey:"<<pubKey<<endl;
            myfile<<"privatekey:"<<privKey<<endl;
            myfile.close();
            last = curr;
            break;
        }
        i++;
        if (i%100000 == 0) {
            cout<<"searched "<<i<<" keys so far"<<endl;
        }
    } 
#ifdef SECP256K1
    secp256k1_context_destroy(ctx);
#endif
}

int main() {
    std::thread key_search_thread(key_search);
    key_search_thread.join();
}