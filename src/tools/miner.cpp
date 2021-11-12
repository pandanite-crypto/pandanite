#include <iostream>
#include <mutex>
#include <thread>
#include <map>
#include <functional>
#include "../core/user.hpp"
#include "../core/api.hpp"
#include "../core/block.hpp"
#include "../core/helpers.hpp"
#include "../core/constants.hpp"
#include "../core/common.hpp"
#include "../core/blockchain.hpp"
#include "../core/crypto.hpp"
using namespace std;


void run_mining(User& miner, string& host) {
    while(true) {
        cout<<"getting problem"<<endl;
        int nextBlock = getCurrentBlockCount(host) + 1;
        json problem = getMiningProblem(host);
        string lastHashStr = problem["lastHash"];
        SHA256Hash lastHash = stringToSHA256(lastHashStr);
        int challengeSize = problem["challengeSize"];
        
        User miner;
        // have miner mine the next block
        Transaction fee = miner.mine(nextBlock);
        vector<Transaction> transactions;
        Block newBlock;
        newBlock.setId(nextBlock);
        newBlock.addTransaction(fee);
        SHA256Hash hash = newBlock.getHash(lastHash);
        SHA256Hash solution = mineHash(hash, challengeSize);
        newBlock.setNonce(solution);
        // send the solution!
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        cout<<"Submitting solution"<<endl;
        cout<<submitBlock(host, newBlock)<<endl;
        
    }
}

int main() {
    json data = readJsonFromFile("./keys/miner.json");
    User miner(data);
    string host = "http://localhost:3000";
    // run miner in background
    std::thread mining_thread(run_mining, ref(miner), ref(host));

    mining_thread.join();
}