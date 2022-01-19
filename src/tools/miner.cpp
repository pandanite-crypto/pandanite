#include "../core/crypto.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/crypto.hpp"
#include "../core/merkle_tree.hpp"
#include "../core/host_manager.hpp"
#include "../core/logger.hpp"
#include "../core/user.hpp"
#include "../core/config.hpp"
#include "../core/worker.hpp"
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <atomic>
#include <chrono>
using namespace std;

vector<Worker*> workers;

#define MAX_CONNECTION_FAILURE_BEFORE_RESET 10

struct block_status {
    std::vector<double> block_hashrates;
    std::mutex _lock;
};

void get_status(block_status& status) {
    time_t start = std::time(0);

    while (true) {
        std::this_thread::sleep_for(std::chrono::minutes(2));

        double net_hashrate = 0;
        
        {
            std::unique_lock<std::mutex> lock(status._lock);
            if (status.block_hashrates.size() > 0) {
                for (auto& hashrate : status.block_hashrates)
                {
                    net_hashrate += hashrate;
                }
                net_hashrate /= status.block_hashrates.size();
            }
        }

        uint32_t accepted = Worker::accepted_blocks.load();
        uint32_t rejected = Worker::rejected_blocks.load();
        uint32_t earned = Worker::earned.load();

        uint64_t hash_count = 0;

        for (size_t i = 0; i < workers.size(); i++) {
            hash_count += workers[i]->get_hash_count();
        }

        if (hash_count == 0) {
            continue;
        }

        auto end = std::time(0);
        auto duration = end - start;
        auto total_blocks = accepted + rejected;

        Logger::beginWriteConsole();
        Logger::writeConsole("Periodic Report");
        if (total_blocks > 0) {
            Logger::writeConsole("Accepted \t\t" + std::to_string(accepted) + " (" + std::to_string(accepted / (double)total_blocks * 100) + "%)");
            Logger::writeConsole("Rejected \t\t" + std::to_string(rejected) + " (" + std::to_string(rejected / (double)total_blocks * 100) + "%)");
        }
        else {
            Logger::writeConsole("Accepted \t\t0 (0%)");
            Logger::writeConsole("Rejected \t\t0 (0%)");
        }
        Logger::writeConsole("Earned \t\t\t" + std::to_string(earned) + " bamboos");
        Logger::writeConsole("Miner hash rate \t" + std::to_string(hash_count / (double)duration / 1000000) + " Mh/sec"); // todo: HARDCODED
        if (net_hashrate > 0) {
            Logger::writeConsole("Net hash rate (est) \t" + std::to_string(net_hashrate / 1000000000) + " Gh/sec"); // todo: HARDCODED
        }
        else {
            Logger::writeConsole("Net hash rate (est) \tN/A");
        }
        Logger::endWriteConsole();
    }
}

void get_work(PublicWalletAddress wallet, HostManager& hosts, block_status& status) {
    TransactionAmount allEarnings = 0;
    int failureCount = 0;
    int last_block_id = 0;
    int last_difficulty = 0;

    time_t blockstart = std::time(0);

    string host = hosts.getGoodHost();

    if (host == "") {
        Logger::logStatus("no host found");
        return;
    }

    while(true) {
        try {
            int retries = 0;
            uint64_t currCount;

            // try 10 times to get block count from current host
            do {
                try {
                    currCount = getCurrentBlockCount(host);
                    break;
                }
                catch (...) { }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                retries++;
            } while (retries < MAX_CONNECTION_FAILURE_BEFORE_RESET);

            // if not found, select new host
            if (retries >= MAX_CONNECTION_FAILURE_BEFORE_RESET) {
                Logger::logStatus("Could not reach host after 10 tries. Connecting to new host...");
                try {
                    host = hosts.getGoodHost();

                    if (host == "") {
                        Logger::logStatus("Refreshing host list...");
                        // refresh hosts and try to get new
                        hosts.refreshHostList();
                        host = hosts.getGoodHost();
                    }
                }
                catch (...) { }

                Logger::logStatus("Connected to host " + host);
                continue;
            }

            if (last_block_id < currCount) {
                std::unique_lock<std::mutex> lock(status._lock);
                if (status.block_hashrates.size() >= 10) {
                    status.block_hashrates.erase(status.block_hashrates.begin());
                }

                if (last_difficulty != 0) {
                    status.block_hashrates.push_back(pow(2, last_difficulty) / (double)(std::time(0) - blockstart));
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            for (size_t i = 0; i < workers.size(); i++) {
                workers[i]->abandon();
            }

            int nextBlock = currCount + 1;
            
            json problem = getMiningProblem(host);

            // download transactions
            vector<Transaction> transactions;
            readRawTransactions(host, transactions);

            Logger::logStatus("[ NEW ] block = " + std::to_string(nextBlock) + ", difficulty = " + to_string(problem["challengeSize"]) + ", transactions = " + to_string(transactions.size()) + " - " + host);

            string lastHashStr = problem["lastHash"];
            SHA256Hash lastHash = stringToSHA256(lastHashStr);
            int challengeSize = problem["challengeSize"];

            // create fee to our wallet:
            Transaction fee(wallet, problem["miningFee"]);
            Block newBlock;

            uint64_t lastTimestamp = std::time(0);
            try { // TODO: remove try catch when all servers are patched with timestamp 
                lastTimestamp = (uint64_t) stringToTime(problem["lastTimestamp"]);

                // check that our mined blocks timestamp is *at least* as old as the tip of the chain.
                // if it's not then your system clock is wonky, so we just make up a date:
                if (newBlock.getTimestamp() < lastTimestamp) {
                    newBlock.setTimestamp(lastTimestamp + 1);
                }
            } catch (...) {}


            newBlock.setId(nextBlock);
            newBlock.addTransaction(fee);


            TransactionAmount total = problem["miningFee"];
            if (newBlock.getTimestamp() < lastTimestamp) {
                newBlock.setTimestamp(lastTimestamp);
            }
        
            for(auto t : transactions) {
                newBlock.addTransaction(t);
                total += t.getTransactionFee();
            }
            
            MerkleTree m;
            m.setItems(newBlock.getTransactions());
            newBlock.setMerkleRoot(m.getRootHash());
            newBlock.setDifficulty(challengeSize);
            newBlock.setLastBlockHash(lastHash);

            last_difficulty = challengeSize;
            last_block_id = currCount;
            blockstart = std::time(0);

            Job job{
                host,
                newBlock
            };
            
            for (size_t i = 0; i < workers.size(); i++) {
                workers[i]->execute(job);
            }
        } catch (const std::exception& e) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

int main(int argc, char **argv) {
    srand(std::time(0));
    json config = getConfig(argc, argv);
    int threads = config["threads"];
    int thread_priority = config["thread_priority"];

    HostManager hosts(config);
    json keys;
    try {
        keys = readJsonFromFile("./keys.json");
    } catch(...) {
        Logger::logStatus("Could not read ./keys.json");
        return 0;
    }
        
    Logger::logStatus("Starting miner with " + to_string(threads) + " threads. Use -t X to change (for example: miner -t 6)");
    
    PublicWalletAddress wallet = stringToWalletAddress(keys["wallet"]);
    Logger::logStatus("Running miner. Coins stored in : " + string(keys["wallet"]));

    block_status status;

    // start worker threads
    for (int i = 0; i < threads; i++) {
        workers.push_back(std::move(new Worker(thread_priority)));
    }

    std::thread status_thread(get_status, ref(status));
    std::thread mining_thread(get_work, wallet, ref(hosts), ref(status));
    mining_thread.join();
}
