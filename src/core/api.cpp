#include <chrono>
#include <iostream>
#include <sstream>
#include "api.hpp"
#include "../external/http.hpp"
#include "constants.hpp"
#include "helpers.hpp"
using namespace std;

uint32_t getCurrentBlockCount(string host_url) {
    http::Request request{host_url + "/block_count"};
    const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
    return std::stoi(std::string{response.body.begin(), response.body.end()});
}

uint64_t getTotalWork(string host_url) {
    http::Request request{host_url + "/total_work"};
    const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
    return std::stol(std::string{response.body.begin(), response.body.end()});
}

string getName(string host_url) {
    http::Request request{host_url + "/name"};
    const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
    return std::string{response.body.begin(), response.body.end()};
}

json getBlockData(string host_url, int idx) {
    http::Request request{host_url + "/block/" + std::to_string(idx)};
    const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
    string responseStr = std::string{response.body.begin(), response.body.end()};
    return json::parse(responseStr);  
}

json addPeerNode(string host_url, string peer_url) {
    http::Request request{host_url + "/add_peer"};
    const auto response = request.send("POST",peer_url,{
        "Content-Type: text/plain"
    },std::chrono::milliseconds{TIMEOUT_MS});
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
    http::Request request(url);
    const auto response = request.send("GET", "", {},std::chrono::milliseconds{TIMEOUT_MS});
    return json::parse(std::string{response.body.begin(), response.body.end()});
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

json verifyTransaction(string host_url, Transaction& t) {
    http::Request request(host_url + "/verify_transaction");
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
    cout<<"|"<<responseStr<<"|"<<endl;
    return json::parse(responseStr);
}




void readRaw(string host_url, int startId, int endId, function<void(Block&)> handler) {
    http::Request request(host_url + "/sync/" + std::to_string(startId) + "/" +  std::to_string(endId) );
    const auto response = request.send("GET", "", {
        "Content-Type: application/octet-stream"
    },std::chrono::milliseconds{TIMEOUT_MS});
    std::vector<char> bytes(response.body.begin(), response.body.end());
    char* buffer = (char*)bytes.data();
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
        if (bytesRead >= bytes.size()) break;
    }
}


void readRawTransactions(string host_url, BloomFilter& bf, function<void(Transaction)> handler) {
    http::Request request(host_url + "/synctx");
    std::pair<char*, size_t> bfs = bf.serialize();
    vector<uint8_t> bytes;
    for(int i = 0; i < bfs.second; i++) {
        bytes.push_back(bfs.first[i]);
    }
    const auto response = request.send("POST", bytes, {
        "Content-Type: application/octet-stream"
    },std::chrono::milliseconds{TIMEOUT_MS});
    
    std::vector<char> responseBytes(response.body.begin(), response.body.end());
    TransactionInfo* curr = (TransactionInfo*)responseBytes.data();
    int numTx = responseBytes.size() / sizeof(TransactionInfo);
    for(int i =0; i < numTx; i++){
        TransactionInfo t = curr[i];
        handler(Transaction(t));
    }
    delete bfs.first;
}


void readRawTransactionsForBlock(string host_url, int blockId, function<void(Transaction)> handler) {
    http::Request request(host_url + "/gettx/" + std::to_string(blockId));
    const auto response = request.send("GET", "", {
        "Content-Type: application/octet-stream"
    },std::chrono::milliseconds{TIMEOUT_MS});
    
    std::vector<char> bytes(response.body.begin(), response.body.end());
    TransactionInfo* curr = (TransactionInfo*)bytes.data();
    int numTx = bytes.size() / sizeof(TransactionInfo);
    for(int i =0; i < numTx; i++){
        TransactionInfo t = curr[i];
        handler(Transaction(t));
    }
}