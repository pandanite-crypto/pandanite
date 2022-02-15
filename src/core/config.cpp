#include "config.hpp"
#include "helpers.hpp"
#include <vector>
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
    }

    it = std::find(args.begin(), args.end(), "--local");
    if (it != args.end()) {
        local = true;
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
