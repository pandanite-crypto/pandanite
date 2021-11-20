#include "../core/crypto.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/user.hpp"
#include "../core/host_manager.hpp"
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include <iostream>
#include <mutex>
#include <thread>
using namespace std;

void simulate_transactions(HostManager& hosts) {
    User miner;
    vector<User> randomUsers;
    for(int i = 0; i < 40; i++) {
        User u;
        randomUsers.push_back(u);
    }
    json data = readJsonFromFile("./keys/miner.json");
    User u(data);
    std::pair<string,int> best = hosts.getLongestChainHost();
    while(true) {
        try {
            if (rand()%1000==0) best = hosts.getLongestChainHost();
            string host = best.first;
            int blockId = getCurrentBlockCount(host) + 2;
            Transaction fee = u.mine(blockId);
            User r = randomUsers[rand()%randomUsers.size()];
            Transaction t = u.send(r, 1 + rand()%5, blockId);
            for (auto newHost : hosts.getHosts()) {
                sendTransaction(newHost, t);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (...) {}
    }
}


int main(int argc, char **argv) {
    string configFile = DEFAULT_CONFIG_FILE_PATH;
    if (argc > 1 ) {
        configFile = string(argv[1]);
    }
    json config = readJsonFromFile(configFile);
    HostManager hosts(config);
    vector<std::thread> requests;
    std::thread sim_thread(simulate_transactions, ref(hosts));
    std::thread sim_thread1(simulate_transactions, ref(hosts));
    std::thread sim_thread2(simulate_transactions, ref(hosts));
    std::thread sim_thread3(simulate_transactions, ref(hosts));
    std::thread sim_thread4(simulate_transactions, ref(hosts));
    std::thread sim_thread5(simulate_transactions, ref(hosts));
    sim_thread.join();
}