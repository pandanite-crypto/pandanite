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

    while(true) {
        cout<<"Enter from user public key:"<<endl;
        string pubKeyStr;
        cin>>pubKeyStr;
        cout<<"Enter from user private key:"<<endl;
        string privKeyStr;
        cin>>privKeyStr;
        PublicKey publicKey = stringToPublicKey(pubKeyStr);
        PrivateKey privateKey = stringToPrivateKey(privKeyStr);

        cout<<"Enter to user wallet:"<<endl;
        string to;
        cin>> to;
        PublicWalletAddress toWallet = stringToWalletAddress(to);
        PublicWalletAddress fromWallet = walletAddressFromPublicKey(publicKey);

        cout<<"Enter the amount:"<<endl;
        TransactionAmount amount;
        cin>>amount;

        cout<<"Enter the mining fee (or 0):"<<endl;
        TransactionAmount fee;
        cin>>fee;
        std::pair<string,int> best = hosts.getBestHost();
        string host = best.first;
        int blockId = getCurrentBlockCount(host) + 1; // usually send 1 block into the future
        Transaction t(fromWallet, toWallet, amount, blockId, publicKey, fee);
        t.sign(publicKey, privateKey);
        cout<<"Creating transaction, for block: "<<blockId<<endl;
        json result = sendTransaction(host, t);
        cout<<result<<endl;
        if(result["status"] != "SUCCESS") {
            cout<<"Transaction failed: " << result["status"]<<endl;
        }
        cout<<"Waiting for block to complete"<<endl;
        while(true) {
            bool added = getCurrentBlockCount(host) >= blockId;
            if (added) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        cout<<"Checking proof"<<endl;
        cout<<verifyTransaction(host, t)<<endl;
        string cont;
        cin>>cont;
    }
}