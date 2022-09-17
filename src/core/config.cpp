#include "config.hpp"
#include "helpers.hpp"
#include "crypto.hpp"
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <thread>
using namespace std;

json getConfig(int argc, char**argv) {
    cout<<"░░░░░░░░▄██▄░░░░░░▄▄░░"<<endl;
    cout<<"░░░░░░░▐███▀░░░░░▄███▌"<<endl;
    cout<<"░░▄▀░░▄█▀▀░░░░░░░░▀██░"<<endl;
    cout<<"░█░░░██░░░░░░░░░░░░░░░"<<endl;
    cout<<"█▌░░▐██░░▄██▌░░▄▄▄░░░▄"<<endl;
    cout<<"██░░▐██▄░▀█▀░░░▀██░░▐▌"<<endl;
    cout<<"██▄░▐███▄▄░░▄▄▄░▀▀░▄██"<<endl;
    cout<<"▐███▄██████▄░▀░▄█████▌"<<endl;
    cout<<"▐████████████▀▀██████░"<<endl;
    cout<<"░▐████▀██████░░█████░░"<<endl;
    cout<<"░░░▀▀▀░░█████▌░████▀░░"<<endl;
    cout<<"░░░░░░░░░▀▀███░▀▀▀░░░░"<<endl;


    std::vector<std::string> args(argv, argv + argc);  
    // should read from config when available
    std::vector<string>::iterator it;
    bool testnet = false;
    bool local = false;
    bool rateLimiter = true;
    bool firewall = false;
    string customWallet = "";
    string customIp = "";
    string customName = randomString(25);
    string networkName = "mainnet";
    json hostSources = json::array();
    json checkpoints = json::array();
    json bannedHashes = json::array();
    int customPort = 3000;
    int threads = std::thread::hardware_concurrency();
    int thread_priority = 0;

    if (threads == 0) {
        threads = 1;
    }

    it = std::find(args.begin(), args.end(), "-t");
    if (it != args.end()) {
        threads = std::stoi(*++it);
    }

    it = std::find(args.begin(), args.end(), "--wallet");
    if (it++ != args.end()) {
        customWallet = string(*it);
    }

    it = std::find(args.begin(), args.end(), "--priority");
    if (it != args.end()) {
        thread_priority = std::stoi(*++it);
    }

    it = std::find(args.begin(), args.end(), "-ip");
    if (it++ != args.end()) {
        customIp = string(*it);
    }


    it = std::find(args.begin(), args.end(), "-n");
    if (it++ != args.end()) {
        customName = string(*it);
    }

    it = std::find(args.begin(), args.end(), "-p");
    if (it++ != args.end()) {
        customPort = std::stoi(*it);
    }

    it = std::find(args.begin(), args.end(), "--network-name");
    if (it++ != args.end()) {
        networkName = string(*it);
    }

    it = std::find(args.begin(), args.end(), "--host-source");
    if (it++ != args.end()) {
        hostSources.push_back(string(*it));
    }

    it = std::find(args.begin(), args.end(), "--testnet");
    if (it != args.end()) {
        testnet = true;
        networkName = "testnet";
        checkpoints.push_back({2, "0E865D520237B975B8D202FABFB25607458AF4605A74361498B24AEA7BF5CB34"});
    } 

    it = std::find(args.begin(), args.end(), "--local");
    if (it != args.end()) {
        networkName = "localnet";
        local = true;
    }

    if (networkName == "mainnet") {
        checkpoints.push_back({1, "0840EF092D16B7D2D31B6F8CBB855ACF36D73F5778A430B0CEDB93A6E33AF750"});
        checkpoints.push_back({7774, "E1DC4CA2F2D634868C12B2C8963B33DD8632F459A1D37701A6B9BE17C0DA99EB"});
        checkpoints.push_back({14142, "BC77EB5157A82E1FC684653FEBECAEAF1034F7E0ABE09A10908B4F75D0F66956"});
        bannedHashes.push_back({143799,"60D84E0D3078F3FFD9E54133B450448F3F12B671028B0431F775A37A347FFACA"});
    }

    it = std::find(args.begin(), args.end(), "--disable-limiter");
    if (it != args.end()) {
        rateLimiter = false;
    }

    it = std::find(args.begin(), args.end(), "--firewall");
    if (it != args.end()) {
        firewall = true;
    }

    json config;
    config["rateLimiter"] = rateLimiter;
    config["threads"] = threads;
    config["wallet"] = customWallet;
    config["port"] = customPort;
    config["name"] = customName;
    config["networkName"] = networkName;
    config["firewall"] = firewall;
    config["checkpoints"] = checkpoints;
    config["bannedHashes"] = bannedHashes;
    config["ip"] = customIp;
    config["thread_priority"] = thread_priority;
    config["hostSources"] = hostSources;
    config["minHostVersion"] = "0.2.4-alpha";
    config["showHeaderStats"] = true;

    if (local) {
        // do nothing
    } else if (testnet) {
        config["hostSources"].push_back("http://173.230.139.86:5000/peers");
    } else {
        config["hostSources"].push_back("http://173.230.139.86:3000/peers");
        config["hostSources"].push_back("http://173.230.139.86:3001/peers");
        config["hostSources"].push_back("http://173.230.139.86:3002/peers");
        config["hostSources"].push_back("http://178.254.42.138:3000/peers");
        config["hostSources"].push_back("http://65.108.218.239:3000/peers");
    }
    return config;
}
