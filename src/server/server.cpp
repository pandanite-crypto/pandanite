#include <App.h>
#include <HttpResponse.h>
#include <functional>
#include <string>
#include <mutex>
#include <thread>
#include <string_view>
#include <atomic>
#include <signal.h>
#include <filesystem>
#include "../core/logger.hpp"
#include "../core/crypto.hpp"
#include "../core/host_manager.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/crypto.hpp"
#include "../core/config.hpp"
#include "request_manager.hpp"
#include "server.hpp"
using namespace std;


void sendCorsHeaders(uWS::HttpResponse<false>* ptr) {
    ptr->writeHeader("Access-Control-Allow-Origin", "*");
    ptr->writeHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    ptr->writeHeader("Access-Control-Allow-Headers", "origin, content-type, accept, x-requested-with");
    ptr->writeHeader("Access-Control-Max-Age", "3600");
}

void checkBuffer(string& buf, uWS::HttpResponse<false>* ptr, uint64_t maxSize=8000000) {
    if (buf.size() > maxSize) ptr->end("Buffer Overflow");
}

void rateLimit(RequestManager& manager, uWS::HttpResponse<false>* ptr) {
    auto remoteAddress = string(ptr->getRemoteAddressAsText());
    if (!manager.acceptRequest(remoteAddress)) ptr->end("Too many requests " + remoteAddress);
}

namespace {
    std::function<void(int)> shutdown_handler;
    void signal_handler(int signal) { 
        shutdown_handler(signal); 
        exit(0);
    }
}

void BambooServer::run(json config) {
    srand(time(0));

    std::filesystem::path data{ "data" };

    if (!std::filesystem::exists(data)) {
        Logger::logStatus("Creating data directory...");
        try {
            std::filesystem::create_directory(data);
        }
        catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }

    Logger::logStatus("Starting server...");
    HostManager hosts(config);

    
    RequestManager manager(hosts);

    // start downloading headers from peers
    hosts.syncHeadersWithPeers();

    // start pinging other peers about ourselves
    hosts.startPingingPeers();


    shutdown_handler = [&](int signal) {
        Logger::logStatus("Shutting down server.");
        manager.exit();
        Logger::logStatus("FINISHED");
    };

    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);


    if (config["rateLimiter"] == false) manager.enableRateLimiting(false);
    
    Logger::logStatus("RequestManager ready...");

    Logger::logStatus("Server Ready.");

    auto corsHandler = [&manager](auto *res, auto *req) {
        sendCorsHeaders(res);
        res->end("");
    };
    
    auto logsHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            string s = "";
            for(auto str : Logger::buffer) {
                s += str + "<br/>";
            }
            res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(s);
        } catch(const std::exception &e) {
            Logger::logError("/logs", e.what());
        } catch(...) {
            Logger::logError("/logs", "unknown");
        }
    };

    auto statsHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            json stats = manager.getStats();
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(stats.dump());
        } catch(const std::exception &e) {
            Logger::logError("/stats", e.what());
        } catch(...) {
            Logger::logError("/stats", "unknown");
        }
    };

    auto totalWorkHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            std::string count = manager.getTotalWork();
            res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(count);
        } catch(const std::exception &e) {
            Logger::logError("/total_work", e.what());
        } catch(...) {
            Logger::logError("/total_work", "unknown");
        }
    };

    auto nameHandler = [&manager, &config](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        json response;
        response["name"] = string(config["name"]);
        response["version"] = BUILD_VERSION;
        response["networkName"] = string(config["networkName"]);
        res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(response.dump());
    };

    auto peerHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        json response = manager.getPeers();
        res->end(response.dump());
    };

    auto blockCountHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            std::string count = manager.getBlockCount();
            res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(count);
        } catch(const std::exception &e) {
            Logger::logError("/block_count", e.what());
        } catch(...) {
            Logger::logError("/block_count", "unknown");
        }
    };

    auto txJsonHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        json result;
        try {
            json result = manager.getTransactionQueue();
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(result.dump());
        } catch(const std::exception &e) {
            result["error"] = "Unknown error";
            Logger::logError("/get_tx_json", e.what());
            res->end("");
        } catch(...) {
            Logger::logError("/get_tx_json", "unknown");
            res->end("");
        }
    };

    auto blockHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        json result;
        try {
            if (req->getQuery("blockId").length() == 0) {
                json err;
                err["error"] = "No query parameters specified";
                res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(err.dump());
                return;
            }
            int blockId= std::stoi(string(req->getQuery("blockId")));
            int count = std::stoi(manager.getBlockCount());
            if (blockId<= 0 || blockId > count) {
                result["error"] = "Invalid Block";
            } else {
                result = manager.getBlock(blockId);
            }
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(result.dump());
        } catch(const std::exception &e) {
            result["error"] = "Unknown error";
            Logger::logError("/block", e.what());
            res->end("");
        } catch(...) {
            Logger::logError("/block", "unknown");
            res->end("");
        }
    };

    auto mineStatusHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        json result;
        try {
            if (req->getQuery("blockId").length() == 0) {
                json err;
                err["error"] = "No query parameters specified";
                res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(err.dump());
                return;
            }
            int blockId = std::stoi(string(req->getQuery("blockId")));
            int count = std::stoi(manager.getBlockCount());
            if (blockId <= 0 || blockId > count) {
                result["error"] = "Invalid Block";
            } else {
                result = manager.getMineStatus(blockId);
            }
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(result.dump());
        } catch(const std::exception &e) {
            result["error"] = "Unknown error";
            Logger::logError("/mine_status", e.what());
            res->end("");
        } catch(...) {
            Logger::logError("/mine_status", "unknown");
            res->end("");
        }
    };

    auto ledgerHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            if (req->getQuery("wallet").length() == 0) {
                json err;
                err["error"] = "No query parameters specified";
                res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(err.dump());
                return;
            }
            PublicWalletAddress w = stringToWalletAddress(string(req->getQuery("wallet")));
            json ledger = manager.getLedger(w);
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(ledger.dump());
        } catch(const std::exception &e) {
            Logger::logError("/ledger", e.what());
        } catch(...) {
            Logger::logError("/ledger", "unknown");
        }
    };

    auto walletHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            if (req->getQuery("wallet").length() == 0) {
                json err;
                err["error"] = "No query parameters specified";
                res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(err.dump());
                return;
            }
            PublicWalletAddress w = stringToWalletAddress(string(req->getQuery("wallet")));
            json ret = manager.getTransactionsForWallet(w);
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(ret.dump());
        } catch(const std::exception &e) {
            Logger::logError("/wallet", e.what());
        } catch(...) {
            Logger::logError("/wallet", "unknown");
        }
    };

    // TODO: remove this once all nodes and clients migrated
    auto ledgerHandlerDeprecated = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            PublicWalletAddress w = stringToWalletAddress(string(req->getParameter(0)));
            json ledger = manager.getLedger(w);
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(ledger.dump());
        } catch(const std::exception &e) {
            Logger::logError("/ledger", e.what());
        } catch(...) {
            Logger::logError("/ledger", "unknown");
        }
    };

    auto createWalletHandler = [&manager](auto *res, auto* readJsonFromFile) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            std::pair<PublicKey,PrivateKey> pair = generateKeyPair();
            PublicKey publicKey = pair.first;
            PrivateKey privateKey = pair.second;
            PublicWalletAddress w = walletAddressFromPublicKey(publicKey);
            string wallet = walletAddressToString(walletAddressFromPublicKey(publicKey));
            string pubKey = publicKeyToString(publicKey);
            string privKey = privateKeyToString(privateKey);
            json key;
            key["wallet"] = wallet;
            key["publicKey"] = pubKey;
            key["privateKey"] = privKey;
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(key.dump());
        } catch (const std::exception& e) {
            Logger::logError("/create_wallet", e.what());
        } catch(...) {
            Logger::logError("/create_wallet", "unknown");
        }
    };

    auto createTransactionHandler = [&manager](auto *res, auto* readJsonFromFile) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        res->onAborted([res]() {
            res->end("ABORTED");
        });
        std::string buffer;
        res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
            buffer.append(data.data(), data.length());
            checkBuffer(buffer, res);
            json txInfo;
            if (last) {
                try {
                    txInfo = json::parse(string(buffer));
                    PublicKey publicKey = stringToPublicKey(txInfo["publicKey"]);
                    PrivateKey privateKey = stringToPrivateKey(txInfo["privateKey"]);    
                    PublicWalletAddress toAddress = stringToWalletAddress(txInfo["to"]);
                    PublicWalletAddress fromAddress = walletAddressFromPublicKey(publicKey);
                    TransactionAmount amount = txInfo["amount"];
                    TransactionAmount fee = txInfo["fee"];
                    uint64_t nonce = txInfo["nonce"];
                    Transaction t(fromAddress, toAddress,amount, publicKey, fee, nonce);
                    t.sign(publicKey, privateKey);
                    json result = t.toJson();
                    res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(result.dump());
                }  catch(const std::exception &e) {
                    Logger::logError("/create_transaction", e.what());
                } catch(...) {
                    Logger::logError("/create_transaction", "unknown");
                }
            }
        });
    };


    auto addPeerHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        res->onAborted([res]() {
            res->end("ABORTED");
        });
        std::string buffer;
        res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
            buffer.append(data.data(), data.length());
            checkBuffer(buffer, res);
            json peerInfo;
            if (last) {
                try {
                    peerInfo = json::parse(string(buffer));
                    // for handling older hosts:
                    if (!peerInfo.contains("networkName")) peerInfo["networkName"] = "mainnet";
                    json result = manager.addPeer(peerInfo["address"], peerInfo["time"], peerInfo["version"], peerInfo["networkName"]);
                    res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(result.dump());
                }  catch(const std::exception &e) {
                    Logger::logStatus(peerInfo.dump());
                    Logger::logError("/add_peer", e.what());
                } catch(...) {
                    Logger::logError("/add_peer", "unknown");
                }
            }
        });
    };


    auto submitHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        res->onAborted([res]() {
            res->end("ABORTED");
        });
        std::string buffer;
        res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
            buffer.append(data.data(), data.length());
            checkBuffer(buffer, res);
            if (last) {
                try {
                    if (buffer.length() < BLOCKHEADER_BUFFER_SIZE + TRANSACTIONINFO_BUFFER_SIZE) {
                        json response;
                        response["error"] = "Malformed block";
                        Logger::logError("/submit","Malformed block");
                        res->end(response.dump());
                    } else {
                        char * ptr = (char*)buffer.c_str();
                        BlockHeader blockH = blockHeaderFromBuffer(ptr);
                        ptr += BLOCKHEADER_BUFFER_SIZE;
                        if (buffer.size() != BLOCKHEADER_BUFFER_SIZE + blockH.numTransactions*TRANSACTIONINFO_BUFFER_SIZE) {
                            json response;
                            response["error"] = "Malformed block";
                            Logger::logError("/submit","Malformed block");
                            res->end(response.dump());
                            return;
                        }

                        vector<Transaction> transactions;
                        if (blockH.numTransactions > MAX_TRANSACTIONS_PER_BLOCK) {
                            json response;
                            response["error"] = "Too many transactions";
                            res->end(response.dump());
                            Logger::logError("/submit","Too many transactions");
                        } else {
                            for(int j = 0; j < blockH.numTransactions; j++) {
                                TransactionInfo t = transactionInfoFromBuffer(ptr);
                                ptr += TRANSACTIONINFO_BUFFER_SIZE;
                                Transaction tx(t);
                                transactions.push_back(tx);
                            }
                            Block block(blockH, transactions);
                            json response = manager.submitProofOfWork(block);
                            res->end(response.dump());
                        }
                    }
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
                
            }
        });
    };

    auto getTxHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            res->writeHeader("Content-Type", "application/octet-stream");
            std::pair<char*, size_t> buffer = manager.getRawTransactionData();
            std::string_view str(buffer.first, buffer.second);
            res->write(str);
            delete buffer.first;
            res->end("");
        } catch(const std::exception &e) {
            Logger::logError("/gettx", e.what());
        } catch(...) {
            Logger::logError("/gettx", "unknown");
        }
        res->onAborted([res]() {
            res->end("ABORTED");
        });
    };

    auto mineHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            json response = manager.getProofOfWork();
            res->end(response.dump());
        } catch(const std::exception &e) {
            Logger::logError("/mine", e.what());
        } catch(...) {
            Logger::logError("/mine", "unknown");
        }
    };

    /************* BEGIN DEPRECATED ***************/

    auto syncHandlerDeprecated = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            int start = std::stoi(string(req->getParameter(0)));
            int end = std::stoi(string(req->getParameter(1)));
            if ((end-start) > BLOCKS_PER_FETCH) {
                Logger::logError("/sync", "invalid range requested");
                res->end("");
            }
            res->writeHeader("Content-Type", "application/octet-stream");
            for (int i = start; i <=end; i++) {
                std::pair<uint8_t*, size_t> buffer = manager.getRawBlockData(i);
                std::string_view str((char*)buffer.first, buffer.second);
                res->write(str);
                delete buffer.first;
            }
            res->end("");
        } catch(const std::exception &e) {
            Logger::logError("/sync", e.what());
        } catch(...) {
            Logger::logError("/sync", "unknown");
        }
        res->onAborted([res]() {
            res->end("ABORTED");
        });
    };

    auto blockHeaderHandlerDeprecated = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            int start = std::stoi(string(req->getParameter(0)));
            int end = std::stoi(string(req->getParameter(1)));
            if ((end-start) > BLOCK_HEADERS_PER_FETCH) {
                Logger::logError("/block_headers", "invalid range requested");
                res->end("");
            }
            res->writeHeader("Content-Type", "application/octet-stream");
            for (int i = start; i <=end; i++) {
                BlockHeader b = manager.getBlockHeader(i);
                char bhBytes[BLOCKHEADER_BUFFER_SIZE];
                blockHeaderToBuffer(b, bhBytes);
                std::string_view str(bhBytes, BLOCKHEADER_BUFFER_SIZE);
                res->write(str);
            }
            res->end("");
        } catch(const std::exception &e) {
            Logger::logError("/block_headers", e.what());
        } catch(...) {
            Logger::logError("/block_headers", "unknown");
        }
        res->onAborted([res]() {
            res->end("ABORTED");
        });
    };

    auto blockHandlerDeprecated = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        json result;
        try {
            int blockId= std::stoi(string(req->getParameter(0)));
            int count = std::stoi(manager.getBlockCount());
            if (blockId<= 0 || blockId > count) {
                result["error"] = "Invalid Block";
            } else {
                result = manager.getBlock(blockId);
            }
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(result.dump());
        } catch(const std::exception &e) {
            result["error"] = "Unknown error";
            Logger::logError("/block", e.what());
            res->end("");
        } catch(...) {
            Logger::logError("/block", "unknown");
            res->end("");
        }
    };

    auto mineStatusHandlerDeprecated = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        json result;
        try {
            int blockId = std::stoi(string(req->getParameter(0)));
            int count = std::stoi(manager.getBlockCount());
            if (blockId <= 0 || blockId > count) {
                result["error"] = "Invalid Block";
            } else {
                result = manager.getMineStatus(blockId);
            }
            res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(result.dump());
        } catch(const std::exception &e) {
            result["error"] = "Unknown error";
            Logger::logError("/block", e.what());
            res->end("");
        } catch(...) {
            Logger::logError("/block", "unknown");
            res->end("");
        }
    };

    /************* END DEPRECATED ***************/

    auto syncHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            if (req->getQuery("start").length() == 0 || req->getQuery("end").length() == 0) {
                json err;
                err["error"] = "No query parameters specified";
                res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(err.dump());
                return;
            }
            int start = std::stoi(string(req->getQuery("start")));
            int end = std::stoi(string(req->getQuery("end")));
            if ((end-start) > BLOCKS_PER_FETCH) {
                Logger::logError("/v2/sync", "invalid range requested");
                res->end("");
            }
            res->writeHeader("Content-Type", "application/octet-stream");
            for (int i = start; i <=end; i++) {
                std::pair<uint8_t*, size_t> buffer = manager.getRawBlockData(i);
                std::string_view str((char*)buffer.first, buffer.second);
                res->write(str);
                delete buffer.first;
            }
            res->end("");
        } catch(const std::exception &e) {
            Logger::logError("/v2/sync", e.what());
        } catch(...) {
            Logger::logError("/v2/sync", "unknown");
        }
        res->onAborted([res]() {
            res->end("ABORTED");
        });
    };

    auto blockHeaderHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            if (req->getQuery("start").length() == 0 || req->getQuery("end").length() == 0) {
                json err;
                err["error"] = "No query parameters specified";
                res->writeHeader("Content-Type", "application/json; charset=utf-8")->end(err.dump());
                return;
            }
            int start = std::stoi(string(req->getQuery("start")));
            int end = std::stoi(string(req->getQuery("end")));
            if ((end-start) > BLOCK_HEADERS_PER_FETCH) {
                Logger::logError("/v2/block_headers", "invalid range requested");
                res->end("");
            }
            res->writeHeader("Content-Type", "application/octet-stream");
            for (int i = start; i <=end; i++) {
                BlockHeader b = manager.getBlockHeader(i);
                char bhBytes[BLOCKHEADER_BUFFER_SIZE];
                blockHeaderToBuffer(b, bhBytes);
                std::string_view str(bhBytes, BLOCKHEADER_BUFFER_SIZE);
                res->write(str);
            }
            res->end("");
        } catch(const std::exception &e) {
            Logger::logError("/v2/block_headers", e.what());
        } catch(...) {
            Logger::logError("/v2/block_headers", "unknown");
        }
        res->onAborted([res]() {
            res->end("ABORTED");
        });
    };

    auto addTransactionHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        res->onAborted([res]() {
            res->end("ABORTED");
        });
        std::string buffer;
        res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
            buffer.append(data.data(), data.length());
            checkBuffer(buffer, res);
            if (last) {
                try {
                    if (buffer.length() < TRANSACTIONINFO_BUFFER_SIZE) {
                        json response;
                        response["error"] = "Malformed transaction";
                        Logger::logError("/add_transaction","Malformed transaction");
                        res->end(response.dump());
                    } else {
                        uint32_t numTransactions = buffer.length() / TRANSACTIONINFO_BUFFER_SIZE;
                        const char* buf = buffer.c_str();
                        json response = json::array();
                        for (int i = 0; i < numTransactions; i++) {
                            TransactionInfo t = transactionInfoFromBuffer(buffer.c_str());
                            Transaction tx(t);
                            response.push_back(manager.addTransaction(tx));
                        }
                        res->end(response.dump());
                    }
                }  catch(const std::exception &e) {
                    Logger::logError("/add_transaction", e.what());
                } catch(...) {
                    Logger::logError("/add_transaction", "unknown");
                }
            }
        });
    };

    auto addTransactionJSONHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        res->onAborted([res]() {
            res->end("ABORTED");
        });
        std::string buffer;
        res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
            buffer.append(data.data(), data.length());
            checkBuffer(buffer, res);
            if (last) {
                try {
                    json parsed = json::parse(string(buffer));
                    json response = json::array();
                    if (parsed.is_array()) {
                        for (auto& item : parsed) {
                            Transaction tx(item);
                            json result;
                            result["txid"] = SHA256toString(tx.hashContents());
                            result["status"] = manager.addTransaction(tx)["status"];
                            response.push_back(result);
                            // only add a maximum of 100 transactions per request
                            if (response.size() > 100) break;
                        }
                    } else {
                        Transaction tx(parsed);
                        json result;
                        result["txid"] = SHA256toString(tx.hashContents());
                        result["status"] = manager.addTransaction(tx)["status"];
                        response.push_back(result);
                    }
                    res->end(response.dump());
                }  catch(const std::exception &e) {
                    Logger::logError("/add_transaction", e.what());
                } catch(...) {
                    Logger::logError("/add_transaction", "unknown");
                }
            }
        });
    };

    auto verifyTransactionHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        res->onAborted([res]() {
            res->end("ABORTED");
        });
        std::string buffer;
        res->onData([res, buffer = std::move(buffer), &manager](std::string_view data, bool last) mutable {
            buffer.append(data.data(), data.length());
            checkBuffer(buffer, res);
            if (last) {
                try {
                    json parsed = json::parse(string(buffer));
                    json response = json::array();
                    if (parsed.is_array()) {
                        for (auto& item : parsed) {
                            response.push_back(manager.getTransactionStatus(stringToSHA256(item["txid"])));
                            // only add a maximum of 100 transactions per request
                            if (response.size() > 100) break;
                        }
                    }
                    res->end(response.dump());
                }  catch(const std::exception &e) {
                    Logger::logError("/verify_transaction", e.what());
                } catch(...) {
                    Logger::logError("/verify_transaction", "unknown");
                }
            }
        });
    };

    auto getNetworkHashrateHandler = [&manager](auto *res, auto *req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        try {
            std::string hashrate = to_string(manager.getNetworkHashrate());
            res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(hashrate);
        } catch(const std::exception &e) {
            Logger::logError("/getnetworkhashrate", e.what());
        } catch(...) {
            Logger::logError("/getnetworkhashrate", "unknown");
        }
    };

    auto mainHandler = [&manager](auto* res, auto*req) {
        rateLimit(manager, res);
        sendCorsHeaders(res);
        string response;
        response +="<head><meta http-equiv=\"refresh\" content=\"1.5\"/><script src=\"https://adamvleggett.github.io/drawdown/drawdown.js\"></script></head>";
        response += "<body>";
        response += "<pre id=\"content\" style=\"display:none\">";
        response += "# Bamboo Server " + string(BUILD_VERSION) + "\n";
        response += "***\n";
        response += "## Stats\n";
        response += "- Loaded Blockchain length: " + manager.getBlockCount() + "\n";
        response += "- Total Work: " + manager.getTotalWork() + "\n";
        response += "***\n";
        response += "## Peers\n";
        json stats = manager.getPeerStats();
        for (auto& peer : stats.items()) {
            response += " - [" + peer.key() + "](" + peer.key() + ") : " + to_string(peer.value()) + "\n";
        }
        response += "***\n";
        response += "</pre><div id=\"visible\"></div>";
        response += "<script>";
        response += "document.getElementById(\"visible\").innerHTML=markdown(document.getElementById(\"content\").innerText)\n";
        response += "</script>";
        response += "</body>";

        res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(response);
    };

 
    uWS::App()
        .get("/", mainHandler)
        .get("/name", nameHandler)
        .get("/total_work", totalWorkHandler)
        .get("/peers", peerHandler)
        .get("/block_count", blockCountHandler)
        .get("/logs", logsHandler)
        .get("/stats", statsHandler)
        .get("/block", blockHandler)
        .get("/tx_json", txJsonHandler)
        .get("/mine_status", mineStatusHandler)
        .get("/ledger", ledgerHandler)
        .get("/wallet_transactions", walletHandler)
        .get("/gettx/:blockId", getTxHandler) // DEPRECATED
        .get("/mine_status/:b", mineStatusHandlerDeprecated) // DEPRECATED
        .get("/ledger/:user", ledgerHandlerDeprecated) // DEPRECATED
        .get("/sync/:start/:end", syncHandlerDeprecated) // DEPRECATED
        .get("/block_headers/:start/:end", blockHeaderHandlerDeprecated) // DEPRECATED
        .get("/block/:b", blockHandlerDeprecated) // DEPRECATED
        .get("/mine", mineHandler)
        .get("/getnetworkhashrate", getNetworkHashrateHandler)
        .post("/add_peer", addPeerHandler)
        .post("/submit", submitHandler)
        .get("/gettx", getTxHandler)
        .get("/sync", syncHandler)
        .get("/block_headers", blockHeaderHandler)
        .get("/synctx", getTxHandler)
        .get("/create_wallet", createWalletHandler)
        .post("/create_transaction", createTransactionHandler)
        .post("/add_transaction", addTransactionHandler)
        .post("/add_transaction_json", addTransactionJSONHandler)
        .post("/verify_transaction", verifyTransactionHandler)
        .options("/name", corsHandler)
        .options("/total_work", corsHandler)
        .options("/peers", corsHandler)
        .options("/block_count", corsHandler)
        .options("/logs", corsHandler)
        .options("/stats", corsHandler)
        .options("/wallet_transactions", corsHandler)
        .options("/block", corsHandler)
        .options("/tx_json", corsHandler)
        .options("/mine_status", corsHandler)
        .options("/ledger", corsHandler)
        .options("/mine", corsHandler)
        .options("/getnetworkhashrate", corsHandler)
        .options("/add_peer", corsHandler)
        .options("/submit", corsHandler)
        .options("/gettx", corsHandler)
        .options("/gettx", corsHandler)
        .options("/sync", corsHandler)
        .options("/block_headers", corsHandler)
        .options("/synctx", corsHandler)
        .options("/add_transaction", corsHandler)
        .options("/add_transaction_json", corsHandler)
        .options("/verify_transaction", corsHandler)
        
        
        .listen((int)config["port"], [&hosts](auto *token) {
            Logger::logStatus("==========================================");
            Logger::logStatus("Started server: " + hosts.getAddress());
            Logger::logStatus("==========================================");
        }).run();
}