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

int TOTAL;
void simulate_transactions(HostManager& hosts) {
    string filepath = "./keys/miner.json";
    User miner(readJsonFromFile(filepath));
    
    vector<User> randomUsers;
    for(int i = 0; i < 40; i++) {
        User u;
        randomUsers.push_back(u);
    }
    std::pair<string,int> best = hosts.getLongestChainHost();
    while(true) {
        try {
            // if (rand()%1000==0) best = hosts.getLongestChainHost();
            string host = hosts.getHosts()[0];
            int blockId = getCurrentBlockCount(host) + 1;
            User r = randomUsers[rand()%randomUsers.size()];
            Transaction t = miner.send(r, 1 + rand()%5, blockId);
            json result = sendTransaction(host, t);
            cout<<"sent: "<< TOTAL<<endl;
            cout<<result.dump()<<endl;
            TOTAL++;
        } catch (...) {
            cout<<"Timed out"<<endl;
        }
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