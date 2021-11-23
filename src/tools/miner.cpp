#include "../core/crypto.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/crypto.hpp"
#include "../core/merkle_tree.hpp"
#include "../core/host_manager.hpp"
#include "../core/logger.hpp"
#include "../core/user.hpp"
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
using namespace std;

void run_mining(PublicWalletAddress wallet, HostManager& hosts) {
    TransactionAmount allEarnings = 0;
    while(true) {
        try {
            std::pair<string,int> bestHost = hosts.getLongestChainHost();
            if (bestHost.first == "") {
                Logger::logStatus("no host found");
            } else {
                Logger::logStatus("fetching problem from " + bestHost.first);
            }
            int bestCount = bestHost.second;
            string host = bestHost.first;
            int nextBlock = bestCount + 1;
            json problem = getMiningProblem(host); 
            Logger::logStatus("Got problem. Difficulty=" + to_string(problem["challengeSize"]));

            // download transactions
            int count = 0;
            vector<Transaction> transactions;
            readRawTransactions(host, [&nextBlock, &count, &transactions](Transaction t) {
                if (t.getBlockId() == nextBlock) {
                    transactions.push_back(t);
                    count++;
                }
            });
            stringstream s;
            s<<"Read "<<count<<" transactions from "<<host;
            Logger::logStatus(s.str());

            string lastHashStr = problem["lastHash"];
            SHA256Hash lastHash = stringToSHA256(lastHashStr);
            int challengeSize = problem["challengeSize"];

            // create fee to our wallet:
            Transaction fee(wallet, nextBlock);
            Block newBlock;
            newBlock.setId(nextBlock);
            newBlock.addTransaction(fee);
            set<string> nonces;
            TransactionAmount total = MINING_FEE;
            if (newBlock.getId() >= MINING_PAYMENTS_UNTIL) {
                total = 0;
            }
            for(auto t : transactions) {
                if (nonces.find(t.getNonce()) != nonces.end()) continue;
                newBlock.addTransaction(t);
                total += t.getTransactionFee();
                nonces.insert(t.getNonce());
            }
            allEarnings += total;
            MerkleTree m;
            m.setItems(newBlock.getTransactions());
            newBlock.setMerkleRoot(m.getRootHash());
            newBlock.setDifficulty(challengeSize);
            SHA256Hash hash = newBlock.getHash(lastHash);
            SHA256Hash solution = mineHash(hash, challengeSize);
            newBlock.setNonce(solution);
            auto result = submitBlock(host, newBlock);
            Logger::logStatus(result.dump(4));
            if (string(result["status"]) == "SUCCESS") {
                Logger::logStatus("================BLOCK ACCEPTED=================");
                Logger::logStatus("Earned:" + std::to_string(total/DECIMAL_SCALE_FACTOR));
                Logger::logStatus("Total:" + std::to_string(allEarnings/DECIMAL_SCALE_FACTOR));
            }
            // std::this_thread::sleep_for(std::chrono::milliseconds(15000));
        } catch (const std::exception& e) {
            Logger::logError("run_mining", string(e.what()));
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}


int main(int argc, char **argv) {
#ifdef SERVER
    string configFile = DEFAULT_CONFIG_FILE_PATH;
    if (argc > 1 ) {
        configFile = string(argv[1]);
    }
    json config = readJsonFromFile(configFile);
    int port = config["port"];
    HostManager hosts(config);
    Logger::logStatus("Running miner");
    string file = "./keys/miner.json";
    User miner(readJsonFromFile(file));
    PublicWalletAddress wallet = miner.getAddress();
#else
    json config;
    config["hostSources"] = json::array();
    config["hostSources"].push_back("http://ec2-34-218-176-84.us-west-2.compute.amazonaws.com/hosts");
    HostManager hosts(config);
    json keys = readJsonFromFile("./keys.json");
    PublicWalletAddress wallet = stringToWalletAddress(keys["wallet"]);
#endif
    Logger::logStatus("Running miner. Coins stored in : " + string(keys["wallet"]));
    std::thread mining_thread(run_mining, wallet, ref(hosts));
    mining_thread.join();
}