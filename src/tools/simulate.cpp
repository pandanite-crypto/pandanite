#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/user.hpp"
#include "../core/config.hpp"
#include "../core/host_manager.hpp"
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include <iostream>
#include <mutex>
#include <thread>
using namespace std;

int TOTAL;
void simulate_transactions(HostManager& hosts) {
    string filepath = "./keys.json";
    User miner(readJsonFromFile(filepath));
    std::pair<string,int> best = hosts.getRandomHost();
    while(true) {
        try {
            if (rand()%1000==0) best = hosts.getRandomHost();
            string host = best.first;
            Transaction t = miner.send(miner, 1 + rand()%15);
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
    srand(std::time(0));
    json config = getConfig(argc, argv);
    HostManager hosts(config);
    vector<std::thread> requests;
    std::thread sim_thread(simulate_transactions, ref(hosts));
    sim_thread.join();
}