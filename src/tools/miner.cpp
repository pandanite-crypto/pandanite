#include "../core/crypto.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/crypto.hpp"
#include "../core/host_manager.hpp"
#include <iostream>
#include <mutex>
#include <thread>
using namespace std;

void run_mining(PublicWalletAddress wallet, HostManager& hosts) {
    while(true) {
        try {
            std::pair<string,int> bestHost = hosts.getLongestChainHost();
            if (bestHost.first == "") continue;
            int bestCount = bestHost.second;
            string host = bestHost.first;
            int nextBlock = bestCount + 1;
            json problem = getMiningProblem(host);
            string lastHashStr = problem["lastHash"];
            SHA256Hash lastHash = stringToSHA256(lastHashStr);
            int challengeSize = problem["challengeSize"];
            
            // create fee to our wallet:
            Transaction fee(wallet, nextBlock);
            vector<Transaction> transactions;
            Block newBlock;
            newBlock.setId(nextBlock);
            newBlock.addTransaction(fee);
            for(auto t : problem["transactions"]) {
                Transaction x(t);
                newBlock.addTransaction(x);
            }
            newBlock.setDifficulty(challengeSize);
            SHA256Hash hash = newBlock.getHash(lastHash);
            SHA256Hash solution = mineHash(hash, challengeSize);
            newBlock.setNonce(solution);
            // send the solution!
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            cout<<"Submitting solution"<<endl;
            cout<<submitBlock(host, newBlock)<<endl;
        } catch (const std::exception& e) {
            cout<<"failure in mining thread: "<<e.what()<<endl;
        }
    }
}


int main(int argc, char **argv) {
    string configFile = DEFAULT_CONFIG_FILE_PATH;
    if (argc > 1 ) {
        configFile = string(argv[1]);
    }
    json config = readJsonFromFile(configFile);
    int port = config["port"];
    HostManager hosts(config);
    cout<<"Running miner"<<endl;
    PublicWalletAddress wallet = walletAddressFromPublicKey(stringToPublicKey(config["minerPublicKey"]));
    std::thread mining_thread(run_mining, wallet, ref(hosts));
    mining_thread.join();
}