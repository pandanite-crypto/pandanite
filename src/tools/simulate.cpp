#include "../core/crypto.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/user.hpp"
#include "../core/crypto.hpp"
#include <iostream>
#include <mutex>
#include <thread>
using namespace std;

void simulate_transactions(vector<string>& hosts) {
    User miner;
    vector<User> randomUsers;
    for(int i = 0; i < 40; i++) {
        User u;
        randomUsers.push_back(u);
    }
    json data = readJsonFromFile("./keys/miner.json");
    User u(data);
    while(true) {
        try {
            string host = hosts[rand()%hosts.size()];
            int blockId = getCurrentBlockCount(host) + 2;
            Transaction fee = u.mine(blockId);
            User r = randomUsers[rand()%randomUsers.size()];
            Transaction t = u.send(r, 1 + rand()%5, blockId);
            cout<<sendTransaction(host, t)<<endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100+rand()%100));
        } catch (...) {}
    }
}


int main(int argc, char **argv) {
    string configFile = DEFAULT_CONFIG_FILE_PATH;
    if (argc > 1 ) {
        configFile = string(argv[1]);
    }
    json config = readJsonFromFile(configFile);
    int port = config["port"];
    vector<string> hosts = config["hosts"];
    std::thread sim_thread(simulate_transactions, ref(hosts));
    sim_thread.join();
}