#include <emscripten/emscripten.h>
#include <functional>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include "../core/logger.hpp"
#include "../core/crypto.hpp"
#include "../core/host_manager.hpp"
#include "../core/helpers.hpp"
#include "../core/api.hpp"
#include "../core/crypto.hpp"
#include "../core/config.hpp"
#include "../server/request_manager.hpp"
#include "../core/request.hpp"

using namespace std;


// https://gist.github.com/WesThorburn/00c47b267a0e8c8431e06b14997778e4

RequestManager* GLOBAL_REQUEST_MANAGER;
HostManager* GLOBAL_HOST_MANAGER;

#ifdef __cplusplus
extern "C" {
#endif

inline const char* cstr(const std::string& message) {
    char * cstr = new char [message.length()+1];
    cstr[message.length()] = 0;
    std::strcpy (cstr, message.c_str());
    return cstr;
}

EMSCRIPTEN_KEEPALIVE const char* receive_result(char* data, uint64_t sz) {
    string s(data);
    string requestId = s.substr(0, REQUEST_ID_LENGTH);
    if (sz != REQUEST_ID_LENGTH) {
        string rawData = string(data+4, sz - REQUEST_ID_LENGTH);
        vector<uint8_t> dataVec(rawData.begin(), rawData.end());
        setResult(requestId, dataVec);
    } else {
        endRequest(requestId);
    }
    return cstr("");
}

EMSCRIPTEN_KEEPALIVE const char* set_address(char* st, uint64_t sz) {
    string addr = string(st, sz);
    GLOBAL_HOST_MANAGER->setAddress(addr);
    string result = addr;
    return cstr(result);
}

EMSCRIPTEN_KEEPALIVE const char* block_count(char* st) {
    string result = to_string(GLOBAL_REQUEST_MANAGER->getBlockCount());
    return cstr(result);
}

EMSCRIPTEN_KEEPALIVE const char* block(char* st) {
    uint32_t block = std::stoi(string(st));
    string result = GLOBAL_REQUEST_MANAGER->getBlock(block).dump();
    return cstr(result);
}

EMSCRIPTEN_KEEPALIVE const char* name(char* st) {
    json response;
    response["name"] = "javascript-node";
    response["version"] = BUILD_VERSION;
    response["networkName"] = "mainnet";
    return cstr(response.dump());
}

EMSCRIPTEN_KEEPALIVE const char* ledger(char* st) {
    string wallet = string(st);
    PublicWalletAddress w = stringToWalletAddress(wallet);
    json ledger = GLOBAL_REQUEST_MANAGER->getLedger(w);
    return cstr(ledger.dump());
}

EMSCRIPTEN_KEEPALIVE const char* total_work(char* st) {
    std::string totalWork = GLOBAL_REQUEST_MANAGER->getTotalWork();
    return cstr(totalWork);
}

EMSCRIPTEN_KEEPALIVE const char* add_peer(char* st) {
    string str = string(st);
    json peerInfo = json::parse(str);
    json result = GLOBAL_REQUEST_MANAGER->addPeer(peerInfo["address"], peerInfo["time"], peerInfo["version"], peerInfo["networkName"]);
    return cstr(result.dump());
}

EMSCRIPTEN_KEEPALIVE const char* stats(char* st) {
    json result = GLOBAL_REQUEST_MANAGER->getStats();
    return cstr(result.dump());
}

EMSCRIPTEN_KEEPALIVE const char* mine_status(char* st) {
    uint32_t blockId = std::stoi(string(st));
    json result = GLOBAL_REQUEST_MANAGER->getMineStatus(blockId);
    return cstr(result.dump());
}

EMSCRIPTEN_KEEPALIVE const char* mine(char* st) {
    string str = string(st);

    return 0;
}

EMSCRIPTEN_KEEPALIVE const char* gettx(char* st) {
    string str = string(st);

    return 0;
}

EMSCRIPTEN_KEEPALIVE const char* sync(char* st) {
    string str = string(st);

    return 0;
}

EMSCRIPTEN_KEEPALIVE const char* block_headers(char* st) {
    string str = string(st);

    return 0;
}

EMSCRIPTEN_KEEPALIVE const char* add_transaction_json(char* st) {
    string str = string(st);

    return 0;
}

EMSCRIPTEN_KEEPALIVE const char* synctx(char* st) {
    string str = string(st);

    return 0;
}

EMSCRIPTEN_KEEPALIVE const char* submit(char* st) {
    string str = string(st);

    return 0;
}

#ifdef __cplusplus
}
#endif


int main(int argc, char** argv) {
    json config = getConfig(argc, argv);
    Logger::logStatus("Starting server...");
    GLOBAL_HOST_MANAGER = new HostManager(config);
    Logger::logStatus("HostManager ready...");
    GLOBAL_REQUEST_MANAGER = new RequestManager(*GLOBAL_HOST_MANAGER);
    MAIN_THREAD_ASYNC_EM_ASM("window.nodeReady=true");
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}