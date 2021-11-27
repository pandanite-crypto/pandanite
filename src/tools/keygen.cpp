#include <iostream>
#include <mutex>
#include <thread>
#include <map>
#include <functional>
#include <fstream>
#include "../core/crypto.hpp"
using namespace std;

int main(int argc, char** argv) {
    cout<<"=====GENERATE WALLET===="<<endl;
    ofstream myfile;
    string filename="./keys.json";
    if (argc > 1) {
        filename = string(argv[1]);
    }
    cout<<"Output will be written to ["<<filename<<"]"<<endl;
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
    cout<<"Output written to "<<filename<<endl;
    json key;
    key["wallet"] = wallet;
    key["publicKey"] = pubKey;
    key["privateKey"] = privKey;
    myfile<<key.dump(4)<<endl;
    myfile.close();
}