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
    cout<<"Setting program for wallet"<<endl;
    cout<<SHA256toString(p.getId())<<endl;
    json result = setProgram(host, p);
    cout<<result<<endl;
}