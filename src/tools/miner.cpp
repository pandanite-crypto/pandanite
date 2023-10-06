#include "../core/crypto.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/crypto.hpp"
#include "../core/merkle_tree.hpp"
#include "../core/host_manager.hpp"
#include "../core/user.hpp"
#include "../core/config.hpp"
#include "spdlog/spdlog.h"
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <atomic>
#include <chrono>
using namespace std;


void get_work(PublicWalletAddress wallet, HostManager& hosts, string& customHostIp) {
    TransactionAmount allEarnings = 0;
    int failureCount = 0;
    int last_block_id = 0;
    int last_difficulty = 0;

    time_t blockstart = std::time(0);

    while(true) {
        try {
            uint64_t currCount;

             string host;
    
            if (customHostIp != "") {
                // mine against a specific IP
                host = customHostIp;

                auto cbc{ getCurrentBlockCount(host)};
                if (!cbc.has_value()) 
                    throw std::runtime_error("Cannot get current block count");
                currCount = *cbc;
            } else {
                host = hosts.getGoodHost();
                currCount = hosts.getBlockCount();
            }

            if (host == "") {
                spdlog::info("no host found");
                return;
            }

            
            
            auto parsed = getMiningProblem(host);
            if (!parsed.has_value()) 
                throw std::runtime_error("Cannot get mining problem from host "s+host+"."s);
            json& problem{*parsed};
            int nextBlock = problem["chainLength"];
            nextBlock++;
            // download transactions
            vector<Transaction> transactions;
            readRawTransactions(host, transactions);

            spdlog::info("[ NEW ] block = {}, difficulty = {}, transactions = {} - {}", std::to_string(nextBlock), to_string(problem["challengeSize"]), to_string(transactions.size()),  host    );

            string lastHashStr = problem["lastHash"];
            SHA256Hash lastHash = stringToSHA256(lastHashStr);
            int challengeSize = problem["challengeSize"];

            // create fee to our wallet:
            Transaction fee(wallet, problem["miningFee"]);
            Block newBlock;

            uint64_t lastTimestamp = std::time(0);

            lastTimestamp = (uint64_t) stringToUint64(problem["lastTimestamp"]);

            // check that our mined blocks timestamp is *at least* as old as the tip of the chain.
            // if it's not then your system clock is wonky, so we just make up a date:
            if (newBlock.getTimestamp() < lastTimestamp) {
                newBlock.setTimestamp(lastTimestamp + 1);
            }

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

            SHA256Hash solution = mineHash(newBlock.getHash(), challengeSize, newBlock.getId() > PUFFERFISH_START_BLOCK);
            newBlock.setNonce(solution);
            spdlog::info("Submitting block...");
            auto result{ submitBlock(host, newBlock)};
            if (!result.has_value()) 
                throw std::runtime_error("Cannot submit block");

            if (result->contains("status") && string((*result)["status"]) == "SUCCESS")  {
                spdlog::info("[ ACCEPTED ]");
            } else {
                spdlog::error("[ REJECTED ]");
                spdlog::error(result->dump(4));
            }          

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        } catch (const std::exception& e) {
            spdlog::error("Exception: "s+e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

int main(int argc, char **argv) {
    srand(std::time(0));
    json config = getConfig(argc, argv);
    int threads = config["threads"];
    int thread_priority = config["thread_priority"];
    string customIp = config["ip"];
    string customWallet = config["wallet"];
    PublicWalletAddress wallet;

    HostManager hosts(config);
    json keys;
    if (customWallet == "") {
        try {
            keys = readJsonFromFile("./keys.json");
        } catch(...) {
            spdlog::error("Could not read ./keys.json");
            return 0;
        }
        wallet = stringToWalletAddress(keys["wallet"]);
        spdlog::info("Running miner. Coins stored in : {}", string(keys["wallet"]));
    } else {
        wallet = stringToWalletAddress(customWallet);
        spdlog::info("Running miner. Coins stored in : {}", customWallet);
    }
        
    spdlog::info("Starting miner on single thread");
    get_work(wallet, hosts, customIp);
}
