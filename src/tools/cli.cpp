#include <map>
#include <iostream>
#include <thread>
#include "../core/user.hpp"
#include "../core/api.hpp"
#include "../core/helpers.hpp"
#include "../core/common.hpp"
#include "../core/host_manager.hpp"
#include "../core/config.hpp"
using namespace std;

int main(int argc, char** argv) {
    
    json config = getConfig(argc, argv);
    HostManager hosts(config);

    json keys;
    try {
        keys = readJsonFromFile("./keys.json");
        
        PublicWalletAddress keyFromAddress = walletAddressFromPublicKey(stringToPublicKey(keys["publicKey"]));
        PublicWalletAddress statedFromAddress = stringToWalletAddress(keys["wallet"]);
        if (keyFromAddress != statedFromAddress) {
            cout<<"Wallet address does not match public key. Keyfile is likely corrupted."<<endl;
            //return 0;
        }
    } catch(...) {
        cout<<"Could not read ./keys.json"<<endl;
        return 0;
    }

    PublicKey publicKey = stringToPublicKey(keys["publicKey"]);
    PrivateKey privateKey = stringToPrivateKey(keys["privateKey"]);

    cout<<"Enter to user wallet:"<<endl;
    string to;
    cin>> to;
    PublicWalletAddress toWallet = stringToWalletAddress(to);
    PublicWalletAddress fromWallet = walletAddressFromPublicKey(publicKey);

    cout<<"Enter the amount in leaves (NOTE: 1 leaf = 1/10,000 BMB):"<<endl;
    TransactionAmount amount;
    cin>>amount;

    cout<<"Enter the mining fee (or 0):"<<endl;
    TransactionAmount fee;
    cin>>fee;

    string host = hosts.getGoodHost();
    
    Transaction t(fromWallet, toWallet, amount,publicKey, fee);
    t.sign(publicKey, privateKey);

    cout<<"Creating transaction..."<<endl;
    cout<<"================================================="<<endl;
    cout<<"TRANSACTION JSON (keep this for your records)"<<endl;
    cout<<"================================================="<<endl;
    cout<<t.toJson().dump()<<endl;
    cout<<"=============================="<<endl;
    json result = sendTransaction(host, t);
    cout<<result<<endl;
    cout<<"Finished."<<endl;
    return 0;
}