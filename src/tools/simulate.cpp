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
    string filepath = "./keys/miner2.json";
    User miner(readJsonFromFile(filepath));
    std::pair<string,int> best = hosts.getBestHost();
    while(true) {
        try {
            // if (rand()%1000==0) best = hosts.getBestHost();
            string host = best.first;
            int blockId = getCurrentBlockCount(host) + 3;
            Transaction t = miner.send(miner, 1 + rand()%5, blockId);
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
    json config;
    config["hostSources"] = json::array();
    config["hostSources"].push_back("http://ec2-34-218-176-84.us-west-2.compute.amazonaws.com/hosts");
    HostManager hosts(config);
    vector<std::thread> requests;
    std::thread sim_thread(simulate_transactions, ref(hosts));
    std::thread sim_thread2(simulate_transactions, ref(hosts));
    std::thread sim_thread3(simulate_transactions, ref(hosts));
    std::thread sim_thread4(simulate_transactions, ref(hosts));
    sim_thread.join();
}