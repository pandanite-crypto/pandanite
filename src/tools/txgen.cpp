#include <map>
#include <iostream>
#include <thread>
#include "../core/user.hpp"
#include "../core/api.hpp"
#include "../core/helpers.hpp"
#include "../core/common.hpp"
#include "../core/host_manager.hpp"
using namespace std;




int main(int argc, char** argv) {
    /*
        Generates a hex encoded transaction 
        Usage: ./txgen <keyfile> <toAddress> <amount> <fee> <blockId>
    */
    if (argc != 6) {
        cout<<"ERROR"<<endl;
    } else {
        try {
            string keyPath = string(argv[1]);
            json keyData = readJsonFromFile(keyPath);
            PublicWalletAddress toWallet = stringToWalletAddress(string(argv[2]));
            
            double rawAmount = std::stod(string(argv[3]));
            TransactionAmount amount =  rawAmount * DECIMAL_SCALE_FACTOR;
            double feeRaw = std::stod(string(argv[4]));
            TransactionAmount fee =  feeRaw * DECIMAL_SCALE_FACTOR;
            int blockId = std::stoi(string(argv[5]));
            PublicKey publicKey = stringToPublicKey(keyData["publicKey"]);
            PublicWalletAddress fromWallet = walletAddressFromPublicKey(publicKey);
            PrivateKey privateKey = stringToPrivateKey(keyData["privateKey"]);
            Transaction t(fromWallet, toWallet, amount, blockId, publicKey, fee);
            t.sign(publicKey, privateKey);
            TransactionInfo raw = t.serialize();
            string hex = hexEncode((char*)&raw, sizeof(raw));
            cout<<hex<<endl;
        } catch(...) {
            cout<<"ERROR"<<endl;
        }
    }
}