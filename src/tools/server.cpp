#include <App.h>
#include <string>
#include <mutex>
#include <thread>
#include <experimental/filesystem>
#include "../core/logger.hpp"
#include "../core/crypto.hpp"
#include "../core/host_manager.hpp"
#include "../core/helpers.hpp"
#include "../core/request_manager.hpp"
#include "../core/api.hpp"
#include "../core/crypto.hpp"
using namespace std;

int main(int argc, char **argv) {
    experimental::filesystem::remove_all(LEDGER_FILE_PATH);
    experimental::filesystem::remove_all(BLOCK_STORE_FILE_PATH);
    
    json config = readJsonFromFile(DEFAULT_CONFIG_FILE_PATH);

    int port = config["port"];

    if (argc > 1) {
        string logfile = string(argv[1]);
        Logger::file.open(logfile);
    }
    HostManager hosts(config);
    RequestManager manager(hosts);
 
    uWS::App().get("/block_count", [&manager](auto *res, auto *req) {
        try {
            std::string count = manager.getBlockCount();
            res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(count);
        } catch(const std::exception &e) {
            Logger::logError("/block_count", e.what());
        } catch(...) {
            Logger::logError("/block_count", "unknown");
        }
    }).get("/stats", [&manager](auto *res, auto *req) {
        try {
            json stats = manager.getStats();
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(stats.dump());
        } catch(const std::exception &e) {
            Logger::logError("/stats", e.what());
        } catch(...) {
            Logger::logError("/stats", "unknown");
        }
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
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(result.dump());
        } catch(const std::exception &e) {
            result["error"] = "Unknown error";
            Logger::logError("/block", e.what());
        } catch(...) {
            Logger::logError("/block", "unknown");
        }
    }).get("/ledger/:user", [&manager](auto *res, auto *req) {
        try {
            PublicWalletAddress w = stringToWalletAddress(string(req->getParameter(0)));
            json ledger = manager.getLedger(w);
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(ledger.dump());
        } catch(const std::exception &e) {
            Logger::logError("/ledger", e.what());
        } catch(...) {
            Logger::logError("/ledger", "unknown");
        }
    }).post("/submit", [&manager](auto *res, auto *req) {
        res->onAborted([res]() {
            res->end("ABORTED");
        });
        try {

            std::string buffer;
            res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
                buffer.append(data.data(), data.length());
                if (last) {
                    json submission = json::parse(buffer);
                    json response = manager.submitProofOfWork(submission);
                    res->end(response.dump());
                }
            });
            
        } catch(const std::exception &e) {
            json response;
            response["error"] = string(e.what());
            res->end(response.dump());
            Logger::logError("/submit", e.what());
        } catch(...) {
            json response;
            response["error"] = "unknown";
            res->end(response.dump());
            Logger::logError("/submit", "unknown");
        }
    }).get("/mine", [&manager](auto *res, auto *req) {
        try {
            json response = manager.getProofOfWork();
            res->end(response.dump());
        } catch(const std::exception &e) {
            Logger::logError("/mine", e.what());
        } catch(...) {
            Logger::logError("/mine", "unknown");
        }
    }).get("/sync/:start/:end", [&manager](auto *res, auto *req) {
        
    }).post("/add_transaction", [&manager](auto *res, auto *req) {
        res->onAborted([res]() {
            res->end("ABORTED");
        });
        try {
            std::string buffer;
            res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
                buffer.append(data.data(), data.length());
                if (last) {
                    json submission = json::parse(buffer);
                    json response = manager.addTransaction(submission);
                    res->end(response.dump());
                }
            });
        }  catch(const std::exception &e) {
            Logger::logError("/add_transaction", e.what());
        } catch(...) {
            Logger::logError("/add_transaction", "unknown");
        }
    }).post("/verify_transaction", [&manager](auto *res, auto *req) {
        /* Allocate automatic, stack, variable as usual */
        std::string buffer;
        /* Move it to storage of lambda */
        res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
            buffer.append(data.data(), data.length());
            if (last) {
                json submission = json::parse(buffer);
                json response = manager.verifyTransaction(submission);
                res->end(response.dump());
            }
        });
        res->onAborted([res]() {
            res->end("ABORTED");
        });
    }).listen(port, [port](auto *token) {
        Logger::logStatus("Started server");
    }).run();

}

