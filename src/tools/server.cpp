#include <emscripten/emscripten.h>
#include <functional>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <sstream>
#include <iterator>
#include <iostream>
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


inline const char* cstr(const std::string& message) {
    char * cstr = new char [message.length()+1];
    cstr[message.length()] = 0;
    std::strcpy (cstr, message.c_str());
    return cstr;
}

template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}



#ifdef __cplusplus
extern "C" {
#endif

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

EMSCRIPTEN_KEEPALIVE const char* serve_result(char* data, uint64_t sz) {
    string s(data, sz);
    Logger::logStatus("Received: " + s);
    // split by ':' to get path + payload
    size_t idx = s.find(":");
    string path = s.substr(1, idx - 1);
    Logger::logStatus("Serving: " + path);
    vector<uint8_t> payload = hexDecode(s.substr(idx + 1));
    string resultStr = "";

    // parse out any query params into dict
    json params;
    size_t paramsIdx = path.find("?");
    if (paramsIdx != string::npos) {
        vector<string> components = split(path.substr(paramsIdx + 1), '&');
        for(auto & pair : components) {
            vector<string> keyValue = split(pair, '=');
            params[keyValue[0]] = keyValue[1];
        }
        Logger::logStatus(params.dump());
        if (path == "ledger") {
            PublicWalletAddress w = stringToWalletAddress(params["wallet"]);
            json ledger = GLOBAL_REQUEST_MANAGER->getLedger(w);
            resultStr = ledger.dump();
        } else if (path == "block_headers") {

        } else if (path == "sync") {

        } else if (path == "block") {

        } else if (path == "mine_status") {

        }
    } else {
        if (path == "name") {
            json response;
            response["name"] = "javascript-node";
            response["version"] = BUILD_VERSION;
            response["networkName"] = "mainnet";
            resultStr = response.dump();
        } else if (path == "block_count") {
            resultStr = GLOBAL_REQUEST_MANAGER->getBlockCount();
        } else if (path == "total_work") {
            resultStr = GLOBAL_REQUEST_MANAGER->getTotalWork();
        } else if (path == "gettx") {

        } else if (path == "peers") {
            json result = GLOBAL_REQUEST_MANAGER->getPeers();
            resultStr = result.dump();
        } else if (path == "mine") {
            json result = GLOBAL_REQUEST_MANAGER->getProofOfWork();
            resultStr = result.dump();
        }  else if (path == "submit") {
        } else if (path == "add_peer") {
        } else if (path == "add_transaction_json") {
        }
    }
    Logger::logStatus("Returning: " + resultStr);
    return cstr(hexEncode(resultStr.c_str(), resultStr.size()));
}

EMSCRIPTEN_KEEPALIVE const char* set_address(char* st, uint64_t sz) {
    string addr = string(st, sz);
    GLOBAL_HOST_MANAGER->setAddress(addr);
    string result = addr;
    return cstr(result);
}

#ifdef __cplusplus
}
#endif


int main(int argc, char** argv) {
    json config = getConfig(argc, argv);
    Logger::logStatus("Starting server...");
    GLOBAL_HOST_MANAGER = new HostManager(config);
    cout<<"HM AT START " <<GLOBAL_HOST_MANAGER<<endl;
    Logger::logStatus("HostManager ready...");
    GLOBAL_REQUEST_MANAGER = new RequestManager(*GLOBAL_HOST_MANAGER);
    MAIN_THREAD_ASYNC_EM_ASM("window.nodeReady=true");
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}