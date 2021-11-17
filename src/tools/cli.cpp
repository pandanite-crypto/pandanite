#include <map>
#include <iostream>
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
        std::pair<string,int> best = hosts.getLongestChainHost();
        string host = best.first;
        int blockId = getCurrentBlockCount(host) + 2; // usually send 1 block into the future
        Transaction t(fromWallet, toWallet, amount, blockId, publicKey, fee);
        t.sign(privateKey);
        cout<<"Creating transaction, current block: "<<blockId<<endl;
        cout<<sendTransaction(host, t)<<endl;
    }
}