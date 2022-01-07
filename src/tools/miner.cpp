#include "../core/crypto.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/crypto.hpp"
#include "../core/merkle_tree.hpp"
#include "../core/host_manager.hpp"
#include "../core/logger.hpp"
#include "../core/user.hpp"
#include "../core/config.hpp"
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <atomic>
#include <chrono>
using namespace std;

void get_host(HostManager& hosts, std::atomic<uint64_t>& latestBlockId) {
    while (true) {
        try {
            std::pair<string, uint64_t> bestHost = hosts.getTrustedHost();
            uint64_t currCount = getCurrentBlockCount(bestHost.first);
            latestBlockId.store(currCount);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        catch (...) {

        }
    }
}

void get_status(miner_status& status) {
    uint64_t start = std::time(0);

    while (true) {
        std::this_thread::sleep_for(std::chrono::minutes(1));

        status._lock.lock();
        auto hash_count = status.hash_count;
        auto accepted = status.accepted_blocks;
        auto rejected = status.rejected_blocks;
        auto earned = status.earned / DECIMAL_SCALE_FACTOR;
        auto difficulty = status.last_difficulty;
        auto block_time = status.last_blocktime;
        status._lock.unlock();

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
        if (block_time > 0) {
            Logger::writeConsole("Net hash rate (est) \t" + std::to_string(pow(2, difficulty) / (double)block_time / 1000000000) + " Gh/sec"); // todo: HARDCODED
        }
        else {
            Logger::writeConsole("Net hash rate (est) \tN/A");
        }
        Logger::endWriteConsole();
    }
}

SHA256Hash start_mining_threads(SHA256Hash target, unsigned char challengeSize, int thread_count, miner_status& status, function<bool()> problemValid) {
    SHA256Hash solution;
    std::atomic<bool> aFound(false);

    vector<thread> threads;

    for (int i = 0; i < thread_count; i++) {
        threads.push_back(std::thread(static_cast<void(*)(SHA256Hash, unsigned char, SHA256Hash&, std::atomic<bool>&, miner_status&, function<bool()>)>(&mineHash), target, challengeSize, ref(solution), ref(aFound), ref(status), problemValid));
    }

    for (int i = 0; i < thread_count; i++) {
        threads[i].join();
    }

    if (aFound.load()) {
        return solution;
    }

    return NULL_SHA256_HASH;
}

void run_mining(PublicWalletAddress wallet, int thread_count, HostManager& hosts, std::atomic<uint64_t>& latestBlockId, miner_status& status) {
    TransactionAmount allEarnings = 0;
    int failureCount = 0;
    while(true) {
        try {
            std::pair<string, uint64_t> bestHost = hosts.getTrustedHost();
            uint64_t currCount = getCurrentBlockCount(bestHost.first);
            if (bestHost.first == "") {
                Logger::logStatus("no host found");
            }

            string host = bestHost.first;
            int nextBlock = currCount + 1;
            json problem = getMiningProblem(host);

            // download transactions
            int count = 0;
            vector<Transaction> transactions;
            readRawTransactions(host, [&nextBlock, &count, &transactions](Transaction t) {
                transactions.push_back(t);
                count++;
            });

            Logger::logStatus("[ NEW ] block = " + std::to_string(nextBlock) + ", difficulty = " + to_string(problem["challengeSize"]) + ", transactions = " + to_string(transactions.size()) + " - " + host);

            string lastHashStr = problem["lastHash"];
            SHA256Hash lastHash = stringToSHA256(lastHashStr);
            int challengeSize = problem["challengeSize"];

            // create fee to our wallet:
            Transaction fee(wallet, problem["miningFee"]);
            Block newBlock;

            
            try {
                uint64_t lastTimestamp = (uint64_t) stringToTime(problem["lastTimestamp"]);

                // check that our mined blocks timestamp is *at least* as old as the tip of the chain.
                // if it's not then your system clock is wonky, so we just make up a date:
                if (newBlock.getTimestamp() < lastTimestamp) {
                    newBlock.setTimestamp(lastTimestamp + 1);
                }
            } catch (...) {}


            newBlock.setId(nextBlock);
            newBlock.addTransaction(fee);

            TransactionAmount total = problem["miningFee"];
            for(auto t : transactions) {
                newBlock.addTransaction(t);
                total += t.getTransactionFee();
            }
            
            MerkleTree m;
            m.setItems(newBlock.getTransactions());
            newBlock.setMerkleRoot(m.getRootHash());
            newBlock.setDifficulty(challengeSize);
            newBlock.setLastBlockHash(lastHash);
            SHA256Hash hash = newBlock.getHash();

            uint64_t block_start = std::time(0);

            SHA256Hash solution = start_mining_threads(hash, challengeSize, thread_count, status, [&nextBlock, &latestBlockId]() {
                return nextBlock == (latestBlockId.load() + 1);
            });

            uint64_t block_end = std::time(0);
            bool accepted;
            uint32_t elapsed;
            nlohmann::json result;

            status._lock.lock();
            status.last_difficulty = problem["challengeSize"].get<uint32_t>();
            status.last_blocktime = block_end - block_start;
            status._lock.unlock();

            if (solution == NULL_SHA256_HASH) {
                continue;
            } else {
                newBlock.setNonce(solution);
                auto transmit_start = std::chrono::steady_clock::now();
                result = submitBlock(host, newBlock);
                elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - transmit_start).count();

                if (string(result["status"]) == "SUCCESS") {
                    accepted = true;
                }
                else {
                    accepted = false;
                }
            }

            status._lock.lock();
            if (accepted) {
                status.earned += total;
                status.accepted_blocks++;
                auto total_blocks = status.accepted_blocks + status.rejected_blocks;
                Logger::logStatus(GREEN + "[ ACCEPTED ] " + RESET + to_string(status.accepted_blocks) + " / " + to_string(status.rejected_blocks) + " (" + to_string(status.accepted_blocks / (double)total_blocks * 100) + ") "+ to_string(elapsed) + "ms");
            }
            else {
                status.rejected_blocks++;
                auto total_blocks = status.accepted_blocks + status.rejected_blocks;
                Logger::logStatus(RED + "[ REJECTED ] " + RESET + to_string(status.accepted_blocks) + " / " + to_string(status.rejected_blocks) + " (" + to_string(status.rejected_blocks / (double)total_blocks * 100) + ") " + to_string(elapsed) + "ms");
                Logger::logStatus(result.dump(4));
            }
            status._lock.unlock();

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (const std::exception& e) {
            Logger::logError("run_mining", string(e.what()));
            failureCount++;
            if (failureCount > 5) {
                hosts.initTrustedHost();
                failureCount = 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

int main(int argc, char **argv) {
    json config = getConfig(argc, argv);
    int threads = config["threads"];

    HostManager hosts(config);
    hosts.getTrustedHost();
    json keys;
    try {
        keys = readJsonFromFile("./keys.json");
    } catch(...) {
        Logger::logStatus("Could not read ./keys.json");
        return 0;
    }
        
    Logger::logStatus("Starting miner with " + to_string(threads) + " thread. Use -t X to change (for example: miner -t 6)");
    
    PublicWalletAddress wallet = stringToWalletAddress(keys["wallet"]);
    Logger::logStatus("Running miner. Coins stored in : " + string(keys["wallet"]));
    std::atomic<uint64_t> latestBlockId;
    
    miner_status status;
    status.accepted_blocks = 0;
    status.rejected_blocks = 0;
    status.earned = 0;
    status.hash_count = 0;
    status.last_blocktime = 0;
    status.last_difficulty = 0;

    std::thread host_thread(get_host, ref(hosts), ref(latestBlockId));
    std::thread status_thread(get_status, ref(status));
    std::thread mining_thread(run_mining, wallet, threads, ref(hosts), ref(latestBlockId), ref(status));
    mining_thread.join();
}