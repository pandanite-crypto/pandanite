#include <iostream>
#include <mutex>
#include <thread>
#include <map>
#include <functional>
#include <fstream>
#include "spdlog/spdlog.h"
#include "../core/crypto.hpp"
using namespace std;

int main(int argc, char** argv) {
    spdlog::info("=====GENERATE WALLET====");
    ofstream myfile;
    string filename="./keys.json";
    if (argc > 1) {
        filename = string(argv[1]);
    }
    spdlog::info("Output will be written to [{}]",filename);
    myfile.open(filename);
    std::pair<PublicKey,PrivateKey> pair = generateKeyPair();
    PublicKey publicKey = pair.first;
    PrivateKey privateKey = pair.second;
    PublicWalletAddress w = walletAddressFromPublicKey(publicKey);
    string wallet = walletAddressToString(walletAddressFromPublicKey(publicKey));
    string pubKey = publicKeyToString(publicKey);
    string privKey = privateKeyToString(privateKey);
    cout<<"Generated Wallet"<<endl;
    cout<<"========================"<<endl;
    cout<<"Wallet Address:"<<wallet<<endl;
    cout<<"Public Key    :"<<pubKey<<endl;
    cout<<"Private Key   :"<<privKey<<endl;
    cout<<"========================"<<endl;
    spdlog::info("Output written to {}",filename);
    json key;
    key["wallet"] = wallet;
    key["publicKey"] = pubKey;
    key["privateKey"] = privKey;
    myfile<<key.dump(4)<<endl;
    myfile.close();
}
