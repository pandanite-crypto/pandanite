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

Bigint getTotalWork(string host_url) {
    http::Request request{host_url + "/total_work"};
    const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
    return Bigint(std::string{response.body.begin(), response.body.end()});
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

json pingPeer(string host_url, string peer_url, uint64_t networkTime, string version) {
    json info;
    info["address"] = peer_url;
    info["time"] = networkTime;
    info["version"] = version;
    http::Request request{host_url + "/add_peer"};
    const auto response = request.send("POST", info.dump(), {
        "Content-Type: text/plain"
    },std::chrono::milliseconds{TIMEOUT_MS});
    string responseStr = std::string{response.body.begin(), response.body.end()};
    return json::parse(responseStr);  
}

json submitBlock(string host_url, Block& block) {
    BlockHeader b = block.serialize();
    vector<uint8_t> bytes(BLOCKHEADER_BUFFER_SIZE + TRANSACTIONINFO_BUFFER_SIZE * b.numTransactions);

    char* ptr = (char*) bytes.data();
    blockHeaderToBuffer(b, ptr);
    ptr += BLOCKHEADER_BUFFER_SIZE;

    for(auto t : block.getTransactions()) {
        TransactionInfo tx = t.serialize();
        transactionInfoToBuffer(tx, ptr);
        ptr += TRANSACTIONINFO_BUFFER_SIZE;
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
    vector<uint8_t> bytes(TRANSACTIONINFO_BUFFER_SIZE);
    transactionInfoToBuffer(info, (char*)bytes.data());
    
    const auto response = request.send("POST", bytes, {
        "Content-Type: application/octet-stream"
    },std::chrono::milliseconds{TIMEOUT_MS * 3});
    std::string responseStr = std::string{response.body.begin(), response.body.end()};
    return json::parse(responseStr);
}

json sendTransactions(string host_url, vector<Transaction>& transactionList) {
    http::Request request(host_url + "/add_transaction");

    vector<uint8_t> bytes(TRANSACTIONINFO_BUFFER_SIZE * transactionList.size());
    for(int i = 0; i < transactionList.size(); i++) {
        TransactionInfo tx = transactionList[i].serialize();
        transactionInfoToBuffer(tx, (char*)bytes.data() + i*TRANSACTIONINFO_BUFFER_SIZE);
    }

    const auto response = request.send("POST", bytes, {
        "Content-Type: application/octet-stream"
    },std::chrono::milliseconds{TIMEOUT_MS * 3});
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

void readRawHeaders(string host_url, int startId, int endId, vector<BlockHeader>& blockHeaders) {
    http::Request request(host_url + "/block_headers/" + std::to_string(startId) + "/" +  std::to_string(endId) );
    const auto response = request.send("GET", "", {
        "Content-Type: application/octet-stream"
    },std::chrono::milliseconds{TIMEOUT_BLOCKHEADERS_MS});

    std::vector<char> bytes(response.body.begin(), response.body.end());
    uint8_t* curr = (uint8_t*)bytes.data();
    int numBlocks = bytes.size() / BLOCKHEADER_BUFFER_SIZE;
    for(int i =0; i < numBlocks; i++){
        blockHeaders.push_back(blockHeaderFromBuffer((char*)curr));
        curr += BLOCKHEADER_BUFFER_SIZE;
    }
}

void readRawBlocks(string host_url, int startId, int endId, vector<Block>& blocks) {
    http::Request request(host_url + "/sync/" + std::to_string(startId) + "/" +  std::to_string(endId) );
    const auto response = request.send("GET", "", {
        "Content-Type: application/octet-stream"
    },std::chrono::milliseconds{TIMEOUT_BLOCK_MS});
    std::vector<char> bytes(response.body.begin(), response.body.end());
    uint8_t* buffer = (uint8_t*)bytes.data();
    uint8_t* currPtr = buffer;
    int bytesRead = 0;
    int i = 0;
    int numBlocks = 0;
    while(true) {
        BlockHeader b = blockHeaderFromBuffer((char*)currPtr);
        vector<Transaction> transactions;
        currPtr += BLOCKHEADER_BUFFER_SIZE;
        bytesRead += BLOCKHEADER_BUFFER_SIZE;
        for(int i = 0; i < b.numTransactions; i++) {
            TransactionInfo tmp = transactionInfoFromBuffer((char*)currPtr);
            transactions.push_back(Transaction(tmp));
            currPtr += TRANSACTIONINFO_BUFFER_SIZE;
            bytesRead += TRANSACTIONINFO_BUFFER_SIZE;
        }
        numBlocks++;
        blocks.push_back(Block(b, transactions));
        if (bytesRead >= bytes.size()) break;
    }
}

void readRawTransactions(string host_url, vector<Transaction>& transactions) {
    http::Request request(host_url + "/gettx");
    const auto response = request.send("GET", "", {
        "Content-Type: application/octet-stream"
    },std::chrono::milliseconds{TIMEOUT_MS});
    
    std::vector<char> bytes(response.body.begin(), response.body.end());
    uint8_t* curr = (uint8_t*)bytes.data();
    int numTx = bytes.size() / TRANSACTIONINFO_BUFFER_SIZE;
    for(int i =0; i < numTx; i++){
        TransactionInfo t = transactionInfoFromBuffer((char*)curr);
        transactions.push_back(Transaction(t));
        curr+= TRANSACTIONINFO_BUFFER_SIZE;
    }
}