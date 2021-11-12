#include <App.h>
#include <string>
#include "../core/crypto.hpp"
#include "../core/request_manager.hpp"
using namespace std;

int main(int argc, char **argv) {
    int port = 3000;
    string chainHost = "";
    if (argc > 1 ) {
        port = std::stoi(string(argv[1]));
    }

    if (argc > 2) {
        chainHost = string(argv[2]);
    }

    RequestManager manager(chainHost);
    uWS::App().get("/block_count", [&manager](auto *res, auto *req) {
        std::string count = manager.getBlockCount();
        res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(count);
    }).get("/block/:b", [&manager](auto *res, auto *req) {
        int idx = std::stoi(string(req->getParameter(0)));
        int count = std::stoi(manager.getBlockCount());
        json result;
        if (idx < 0 || idx > count) {
            result["error"] = "Invalid Block";
        } else {
            result = manager.getBlock(idx);
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

