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
    map<string,User*> users;
    string configFile = DEFAULT_CONFIG_FILE_PATH;
    if (argc > 1 ) {
        configFile = string(argv[1]);
    }
    json config = readJsonFromFile(configFile);
    int port = config["port"];
    HostManager hosts(config);

    json keys;
    try {
        keys = readJsonFromFile("./keys.json");
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

    cout<<"Enter the amount:"<<endl;
    TransactionAmount amount = 10000;
    cin>>amount;

    cout<<"Enter the mining fee (or 0):"<<endl;
    TransactionAmount fee;
    cin>>fee;
    std::pair<string,int> best = hosts.getBestHost();
    string host = best.first;
    
    int dummy = 0;
    Transaction t(fromWallet, toWallet, amount, dummy, publicKey, fee);
    t.sign(publicKey, privateKey);
    cout<<"Creating transaction..."<<endl;
    json result = sendTransaction(host, t);
    cout<<result<<endl;
    if(result["status"] != "SUCCESS") {
        cout<<"Transaction failed: " << result["status"]<<endl;
    }
    cout<<"Finished."<<endl;
}