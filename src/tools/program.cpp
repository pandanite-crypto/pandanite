#include <map>
#include <iostream>
#include <thread>
#include "../shared/user.hpp"
#include "../shared/api.hpp"
#include "../core/helpers.hpp"
#include "../server/program.hpp"
#include "../core/common.hpp"
#include "../shared/host_manager.hpp"
#include "../shared/config.hpp"
using namespace std;

const char* KEY = R""""( {
    "privateKey": "08C8D4DAE9A4AE9C456BBB4289883D766DF96DE7E7F44E8F6AC4864A3B9E58539C8307267EF73FD0170145982AE6C0DF14BBFB3355CF3FCEBCE998ECE3BB820D",
    "publicKey": "8F53331361C885D1E9ED3BA0AE465226AE368BA8DB3DCC4875B2D98FB0AE5B01",
    "wallet": "00CF74F1F0111F43E305283219C9DF908D70A14A528083B77A"
})"""";


int main(int argc, char** argv) {
    
    json config = getConfig(argc, argv);
    config["showHeaderStats"] = false;
    config["storagePath"] = "./test-data/prog_";
    HostManager hosts(config);
    hosts.syncHeadersWithPeers();
    string host = hosts.getGoodHost();
    cout<<"reading byte code"<<endl;
    vector<uint8_t> byteCode = readBytes("src/wasm/simple_nft.wasm");
    cout<<"loaded byte code"<<endl;
    Program p(byteCode, config);
    cout<<"Uploading program "<<endl;
    json result = setProgram(host, p);

    cout<<"Transferring NFT"<<endl;
    json keys = json::parse(string(KEY));
    User sender(keys);
    User receiver;
    Transaction tx = sender.send(receiver, 1);
    tx.setProgramId(p.getId());
    cout<<"PROGRAM ID IS: " << SHA256toString(p.getId()) <<endl;
    tx.sign(sender.getPublicKey(), sender.getPrivateKey());
    cout<<"================================================="<<endl;
    cout<<"TRANSACTION JSON (keep this for your records)"<<endl;
    cout<<"================================================="<<endl;
    cout<<tx.toJson().dump()<<endl;
    cout<<"=============================="<<endl;
    result = sendTransaction(host, tx);
    cout<<result<<endl;
    cout<<"Finished."<<endl;
    
}