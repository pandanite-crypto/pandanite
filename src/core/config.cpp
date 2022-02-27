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
    string customWallet = "";
    string customIp = "";
    string customName = randomString(25);
    string networkName = "mainnet";
    json checkpoints = json::array();
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

    it = std::find(args.begin(), args.end(), "--testnet");
    if (it != args.end()) {
        testnet = true;
        networkName = "testnet";
    } 

    it = std::find(args.begin(), args.end(), "--local");
    if (it != args.end()) {
        networkName = "localnet";
        local = true;
        checkpoints.push_back({2, "C80BEA1DF44207E3A46456627E75E02F62087DE8AAD5AFD58DD58689396C2396"});
    }

    if (!local && !testnet) {
        networkName = "mainnet";
        checkpoints.push_back({1, "0840EF092D16B7D2D31B6F8CBB855ACF36D73F5778A430B0CEDB93A6E33AF750"});
        checkpoints.push_back({7774, "E1DC4CA2F2D634868C12B2C8963B33DD8632F459A1D37701A6B9BE17C0DA99EB"});
        checkpoints.push_back({14142, "BC77EB5157A82E1FC684653FEBECAEAF1034F7E0ABE09A10908B4F75D0F66956"});
    }

    it = std::find(args.begin(), args.end(), "--disable-limiter");
    if (it != args.end()) {
        rateLimiter = false;
    }

    json config;
    config["rateLimiter"] = rateLimiter;
    config["threads"] = threads;
    config["wallet"] = customWallet;
    config["port"] = customPort;
    config["name"] = customName;
    config["networkName"] = networkName;
    config["checkpoints"] = checkpoints;
    config["ip"] = customIp;
    config["thread_priority"] = thread_priority;
    config["hostSources"] = json::array();

    if (local) {
        // do nothing
    } else if (testnet) {
        config["hostSources"].push_back("http://54.185.169.234:3000/peers");
    } else {
        config["hostSources"].push_back("http://ec2-35-84-249-159.us-west-2.compute.amazonaws.com:3000/peers");
        config["hostSources"].push_back("http://ec2-44-227-179-62.us-west-2.compute.amazonaws.com:3000/peers");
        config["hostSources"].push_back("http://ec2-54-189-82-240.us-west-2.compute.amazonaws.com:3000/peers");
    }
    return config;
}
