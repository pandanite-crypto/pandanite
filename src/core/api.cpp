#include <chrono>
#include <iostream>
#include <curl/curl.h>
#include <sstream>
#include "api.hpp"
#include "../external/http.hpp"
#include "constants.hpp"
#include "helpers.hpp"
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
struct DataHandler {
    function<void(BlockHeader,vector<Transaction>&)> callback;
    BlockHeader block;
    bool headerRead;
    int totalBytes;
    void * startPtr;
    stringstream soFar;

};

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    string data((const char*) ptr, (size_t) size * nmemb);
    DataHandler * handler = (DataHandler*) stream;
    handler->soFar<<data;
    if (!handler->headerRead && handler->soFar.str().length() >= sizeof(BlockHeader)) {
        memcpy(&handler->block, handler->soFar.str().c_str(), sizeof(BlockHeader));
        handler->headerRead = true;
        handler->totalBytes = sizeof(BlockHeader) + sizeof(TransactionInfo) * handler->block.numTransactions;
    } else if(handler->soFar.str().length() >= handler->totalBytes) {
        string s = handler->soFar.str();
        handler->soFar.str("");
        handler->soFar<<s.substr(handler->totalBytes);
        char * buffer = (char*)s.c_str() + sizeof(BlockHeader);
        vector<Transaction> transactions;
        for(int i = 0; i < handler->block.numTransactions; i++) {
            TransactionInfo tmp;
            memcpy(&tmp, buffer, sizeof(TransactionInfo));
            buffer += sizeof(TransactionInfo);
            transactions.push_back(Transaction(tmp));
        }
        handler->startPtr = buffer + sizeof(TransactionInfo);
        handler->headerRead = false;
        handler->callback(handler->block, transactions);
    }
    return size * nmemb;
}

void readRaw(string host_url, int startId, int endId, function<void(BlockHeader,vector<Transaction>&)> handler) {
    CURL *curl;
    std::string readBuffer;
    stringstream url;
    url<<host_url<<"/sync/"<<startId<<"/"<<endId;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); 
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");
    DataHandler d;
    d.headerRead = false;
    d.callback = handler;
    d.totalBytes = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &d);
    /* Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
}