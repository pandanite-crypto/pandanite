#include <App.h>
#include <string>
#include <mutex>
#include <thread>
#include <experimental/filesystem>
#include "../core/crypto.hpp"
#include "../core/helpers.hpp"
#include "../core/request_manager.hpp"
#include "../core/api.hpp"
#include "../core/crypto.hpp"
using namespace std;

void run_mining(PublicWalletAddress wallet, vector<string>& hosts) {
    while(true) {
        string bestHost = "";
        int bestCount = 0;
        for(auto host : hosts) {
            try {
                int curr = getCurrentBlockCount(host);
                if (curr > bestCount) {
                    bestCount = curr;
                    bestHost = host;
                }
            } catch (...) {
                cout<<"Host failed: "<<host<<endl;
            }
        }
        if (bestHost == "") continue;
        cout<<"getting problem"<<endl;
        string host = bestHost;
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
    }
}

int main(int argc, char **argv) {
    experimental::filesystem::remove_all(LEDGER_FILE_PATH);
    experimental::filesystem::remove_all(BLOCK_STORE_FILE_PATH);
    string configFile = DEFAULT_CONFIG_FILE_PATH;
    if (argc > 1 ) {
        configFile = string(argv[1]);
    }

    json config = readJsonFromFile(configFile);

    int port = config["port"];
    vector<string> hosts = config["hosts"];
    bool runMiner = config["runMiner"];
    if (runMiner) {
        cout<<"Running miner"<<endl;
        PublicWalletAddress wallet = walletAddressFromPublicKey(stringToPublicKey(config["minerPublicKey"]));
        std::thread mining_thread(run_mining, wallet, ref(hosts));
        mining_thread.detach();
    }

    RequestManager manager(hosts);
    uWS::App().get("/block_count", [&manager](auto *res, auto *req) {
        std::string count = manager.getBlockCount();
        res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(count);
    }).get("/stats", [&manager](auto *res, auto *req) {
        json stats = manager.getStats();
        res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(stats.dump());
    }).get("/block/:b", [&manager](auto *res, auto *req) {
        json result;
        try {
            int idx = std::stoi(string(req->getParameter(0)));
            int count = std::stoi(manager.getBlockCount());
            if (idx < 0 || idx > count) {
                result["error"] = "Invalid Block";
            } else {
                result = manager.getBlock(idx);
            }
        } catch (...) {
            result["error"] = "Unknown error";
        }
        res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(result.dump());
    }).get("/ledger/:user", [&manager](auto *res, auto *req) {
        PublicWalletAddress w = stringToWalletAddress(string(req->getParameter(0)));
        json ledger = manager.getLedger(w);
        res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(ledger.dump());
    }).post("/submit", [&manager](auto *res, auto *req) {
        std::string buffer;
        res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
            buffer.append(data.data(), data.length());
            if (last) {
                json submission = json::parse(buffer);
                json response = manager.submitProofOfWork(submission);
                res->end(response.dump());
            }
        });
        res->onAborted([]() {});
        /* Unwind stack, delete buffer, will just skip (heap) destruction since it was moved */
    }).get("/mine", [&manager](auto *res, auto *req) {
            json response = manager.getProofOfWork();
            res->end(response.dump());
    }).post("/add_transaction", [&manager](auto *res, auto *req) {
        /* Allocate automatic, stack, variable as usual */
        std::string buffer;
        /* Move it to storage of lambda */
        res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
            buffer.append(data.data(), data.length());
            if (last) {
                json submission = json::parse(buffer);
                json response = manager.addTransaction(submission);
                res->end(response.dump());
            }
        });
        res->onAborted([]() {});
    }).listen(port, [port](auto *token) {
        cout<<"Started server on port "<<port<<endl;
    }).run();

}

