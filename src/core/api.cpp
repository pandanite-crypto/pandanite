#include "api.hpp"
#include "../external/http.hpp"
#include "constants.hpp"
#include "helpers.hpp"
#include <chrono>
#include <iostream>
 using namespace std;

int getCurrentBlockCount(string host_url) {
    http::Request request{host_url + "/block_count"};
    const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
    string count = std::string{response.body.begin(), response.body.end()};
    return std::stoi(std::string{response.body.begin(), response.body.end()});
}

json getBlockData(string host_url, int idx) {
    http::Request request{host_url + "/block/" + std::to_string(idx)};
    const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
    string responseStr = std::string{response.body.begin(), response.body.end()};
    return json::parse(responseStr);  
}

json submitBlock(string host_url, Block& b) {
    json submission;
    submission["block"] = b.toJson();
    string miningTransaction = submission.dump();
    http::Request request(host_url + "/submit");
    const auto response = request.send("POST", miningTransaction, {
        "Content-Type: application/json"
    }, std::chrono::milliseconds{TIMEOUT_MS});
    return json::parse(std::string{response.body.begin(), response.body.end()});
}

json getMiningProblem(string host_url) {
    http::Request request{host_url + "/mine"};
    const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
    return json::parse(std::string{response.body.begin(), response.body.end()});
}

json sendTransaction(string host_url, Transaction& t) {
    http::Request request(host_url + "/add_transaction");
    json req;
    req["transaction"] = t.toJson();
    const auto response = request.send("POST", req.dump(), {
        "Content-Type: application/json"
    },std::chrono::milliseconds{TIMEOUT_MS});
    std::string responseStr = std::string{response.body.begin(), response.body.end()};
    return json::parse(responseStr);
}