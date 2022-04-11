#include <chrono>
#include <iostream>
#include <sstream>
#include "api.hpp"
#include "constants.hpp"
#include "helpers.hpp"
#include "request.hpp"
using namespace std;

uint32_t getCurrentBlockCount(string host_url) {
    const auto response = sendGetRequest(host_url + "/block_count", TIMEOUT_MS);
    return std::stoi(std::string{response.begin(), response.end()});
}

Bigint getTotalWork(string host_url) {
    const auto response = sendGetRequest(host_url + "/total_work", TIMEOUT_MS);
    return Bigint(std::string{response.begin(), response.end()});
}

json getName(string host_url) {
    const auto response = sendGetRequest(host_url + "/name", TIMEOUT_MS);
    return json::parse(std::string{response.begin(), response.end()});
}

json getBlockData(string host_url, int idx) {
    string url = host_url + "/block/" + std::to_string(idx);
    const auto response = sendGetRequest(url, TIMEOUT_MS);
    string responseStr = std::string{response.begin(), response.end()};
    return json::parse(responseStr);  
}

json pingPeer(string host_url, string peer_url, uint64_t networkTime, string version, string networkName) {
    json info;
    info["address"] = peer_url;
    info["networkName"] = networkName;
    info["time"] = networkTime;
    info["version"] = version;
    string data = info.dump();
    vector<uint8_t> content(data.begin(), data.end());
    auto response = sendPostRequest(host_url + "/add_peer", TIMEOUT_MS, content);
    string responseStr = std::string{response.begin(), response.end()};
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
    auto response = sendPostRequest(host_url + "/submit", TIMEOUT_SUBMIT_MS, bytes);
    return json::parse(std::string{response.begin(), response.end()});
}

json sendTransaction(string host_url, Transaction& t) {
    TransactionInfo info = t.serialize();
    vector<uint8_t> bytes(TRANSACTIONINFO_BUFFER_SIZE);
    transactionInfoToBuffer(info, (char*)bytes.data());
    auto response = sendPostRequest(host_url + "/add_transaction", TIMEOUT_MS * 3, bytes);
    std::string responseStr = std::string{response.begin(), response.end()};
    return json::parse(responseStr);
}

void readRawHeaders(string host_url, int startId, int endId, vector<BlockHeader>& blockHeaders) {
    auto response = sendGetRequest(host_url + "/block_headers?start=" + std::to_string(startId) + "&end=" +  std::to_string(endId), TIMEOUT_BLOCKHEADERS_MS);
    std::vector<char> bytes(response.begin(), response.end());
    uint8_t* curr = (uint8_t*)bytes.data();
    int numBlocks = bytes.size() / BLOCKHEADER_BUFFER_SIZE;
    for(int i =0; i < numBlocks; i++){
        blockHeaders.push_back(blockHeaderFromBuffer((char*)curr));
        curr += BLOCKHEADER_BUFFER_SIZE;
    }
}

void readRawBlocks(string host_url, int startId, int endId, vector<Block>& blocks) {
    auto response = sendGetRequest(host_url + "/sync?start=" + std::to_string(startId) + "&end=" +  std::to_string(endId), TIMEOUT_BLOCK_MS );
    std::vector<char> bytes(response.begin(), response.end());
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
    auto response = sendGetRequest(host_url + "/gettx", TIMEOUT_MS);
    std::vector<char> bytes(response.begin(), response.end());
    uint8_t* curr = (uint8_t*)bytes.data();
    int numTx = bytes.size() / TRANSACTIONINFO_BUFFER_SIZE;
    for(int i =0; i < numTx; i++){
        TransactionInfo t = transactionInfoFromBuffer((char*)curr);
        transactions.push_back(Transaction(t));
        curr+= TRANSACTIONINFO_BUFFER_SIZE;
    }
}