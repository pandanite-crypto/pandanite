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

json submitBlock(string host_url, Block& block) {
    BlockHeader b = block.serialize();
    vector<uint8_t> bytes;
    char * ptr = (char*)&b;
    for(int i = 0; i < sizeof(BlockHeader); i++) {
        bytes.push_back(*ptr);
        ptr++;
    }
    for(auto t : block.getTransactions()) {
        TransactionInfo tx = t.serialize();
        char* ptr = (char*)&tx;
        for(int j = 0; j < sizeof(TransactionInfo); j++) {
            bytes.push_back(*ptr);
            ptr++;
        }
    }
    http::Request request(host_url + "/submit");
    const auto response = request.send("POST", bytes, {
        "Content-Type: application/octet-stream"
    }, std::chrono::milliseconds{TIMEOUT_SUBMIT_MS});
    return json::parse(std::string{response.body.begin(), response.body.end()});
}

json getMiningProblem(string host_url) {
    string url = host_url + "/mine";
    string response = curlGet(url);
    return json::parse(response);
}

json sendTransaction(string host_url, Transaction& t) {
    http::Request request(host_url + "/add_transaction");
    TransactionInfo info = t.serialize();
    vector<uint8_t> bytes;
    char * ptr = (char*)&info;
    for(int i = 0; i < sizeof(TransactionInfo); i++) {
        bytes.push_back(*ptr);
        ptr++;
    }
    const auto response = request.send("POST", bytes, {
        "Content-Type: application/octet-stream"
    },std::chrono::milliseconds{TIMEOUT_MS});
    std::string responseStr = std::string{response.body.begin(), response.body.end()};
    return json::parse(responseStr);
}

struct DataHandler {
    int totalBytes;
    stringstream soFar;
};


size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    string data((const char*) ptr, (size_t) size * nmemb);
    DataHandler * handler = (DataHandler*) stream;
    handler->soFar<<data;
    handler->totalBytes+=nmemb;
    return size * nmemb;
}


void readRaw(string host_url, int startId, int endId, function<void(Block&)> handler) {
    CURL *curl;
    std::string readBuffer;
    stringstream url;
    url<<host_url<<"/sync/"<<startId<<"/"<<endId;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); 
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    DataHandler d;
    d.soFar = stringstream();
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
    string st = d.soFar.str();
    char* buffer = (char*)st.c_str();
    char* currPtr = buffer;
    int bytesRead = 0;
    int i = 0;
    int numBlocks = 0;
    while(true) {
        BlockHeader b;
        vector<Transaction> transactions;
        memcpy(&b, currPtr, sizeof(BlockHeader));
        currPtr += sizeof(BlockHeader);
        bytesRead += sizeof(BlockHeader);
        for(int i = 0; i < b.numTransactions; i++) {
            TransactionInfo tmp;
            memcpy(&tmp, currPtr, sizeof(TransactionInfo));
            transactions.push_back(Transaction(*((TransactionInfo*)currPtr)));
            currPtr += sizeof(TransactionInfo);
            bytesRead += sizeof(TransactionInfo);
        }
        numBlocks++;
        Block block(b, transactions);
        handler(block);
        if (bytesRead >= st.size()) break;
    }
    curl_easy_cleanup(curl);
}


void readRawTransactions(string host_url, function<void(Transaction)> handler) {
    CURL *curl;
    std::string readBuffer;
    stringstream url;
    url<<host_url<<"/synctx";
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); 
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    DataHandler d;
    d.soFar = stringstream();
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
    string st = d.soFar.str();
    char* buffer = (char*)st.c_str();
    TransactionInfo* curr = (TransactionInfo*)buffer;
    int numTx = d.totalBytes / sizeof(TransactionInfo);
    for(int i =0; i < numTx; i++){
        TransactionInfo t = curr[i];
        handler(Transaction(t));
        i++;
    }
    curl_easy_cleanup(curl);
}