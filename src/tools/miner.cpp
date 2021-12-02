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

void get_status( HostManager& hosts, std::mutex& statusLock, uint64_t& latestBlockId) {
    while(true) {
        try {
            std::pair<string,uint64_t> bestHost = hosts.getBestHost();
            statusLock.lock();
            latestBlockId = bestHost.second;
            statusLock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch(...) {
        }
    }
}


void run_mining(PublicWalletAddress wallet, HostManager& hosts, std::mutex& statusLock, uint64_t& latestBlockId) {
    TransactionAmount allEarnings = 0;
    BloomFilter bf;
    while(true) {
        try {
            std::pair<string,int> bestHost = hosts.getBestHost();
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
            readRawTransactionsForBlock(host, nextBlock, [&nextBlock, &count, &transactions](Transaction t) {
                transactions.push_back(t);
                count++;
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
            
            MerkleTree m;
            m.setItems(newBlock.getTransactions());
            newBlock.setMerkleRoot(m.getRootHash());
            newBlock.setDifficulty(challengeSize);
            newBlock.setLastBlockHash(lastHash);
            SHA256Hash hash = newBlock.getHash();
            bool doSubmit = true;
            SHA256Hash solution = mineHash(hash, challengeSize, [&statusLock, &nextBlock, &latestBlockId, &doSubmit]() {
                statusLock.lock();
                if (nextBlock != (latestBlockId + 1)) {
                    doSubmit = false;
                }
                bool ret = nextBlock == (latestBlockId + 1);
                statusLock.unlock();
                return ret;
            });
            if (!doSubmit) {
                Logger::logStatus("==============BLOCK ALREADY SOLVED==============");
                Logger::logStatus("Stopping mining and fetching new block.");   
                continue;
            }
            newBlock.setNonce(solution);
            auto result = submitBlock(host, newBlock);
            Logger::logStatus(result.dump(4));
            if (string(result["status"]) == "SUCCESS") {
                allEarnings += total;
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
    string file = "./keys/miner2.json";
    json info = readJsonFromFile(file);
    PublicWalletAddress wallet = stringToWalletAddress(info["wallet"]);
    Logger::logStatus("Mining to wallet: " + walletAddressToString(wallet));
#else
    json config;
    config["hostSources"] = json::array();
    config["hostSources"].push_back("http://ec2-34-218-176-84.us-west-2.compute.amazonaws.com/hosts");
    HostManager hosts(config);
    json keys = readJsonFromFile("./keys.json");
    PublicWalletAddress wallet = stringToWalletAddress(keys["wallet"]);
    Logger::logStatus("Running miner. Coins stored in : " + string(keys["wallet"]));
#endif
    std::mutex statusLock;
    uint64_t latestBlockId;
    std::thread status_thread(get_status, ref(hosts), ref(statusLock), ref(latestBlockId));
    std::thread mining_thread(run_mining, wallet, ref(hosts), ref(statusLock), ref(latestBlockId));
    mining_thread.join();
}