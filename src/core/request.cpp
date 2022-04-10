#include "request.hpp"
#include "logger.hpp"
#include "helpers.hpp"
#include "crypto.hpp"
#include <thread>
#include <map>
#include <emscripten/fetch.h>
using namespace std;
/*
    TODO: these will send call a javascript function which will block until the request completes
    and will send back the data
*/

std::mutex requestLock;
std::map<RequestID, vector<uint8_t>> results;

RequestID sendRequest(string& url, vector<uint8_t> &data, uint32_t timeoutMSecs) {
    json task;
    task["taskId"] = randomString(REQUEST_ID_LENGTH);
    task["url"] = url;
    task["data"] = hexEncode((const char*)data.data(), data.size());
    task["timeout"] = timeoutMSecs;
    MAIN_THREAD_EM_ASM("window.sendTask(UTF8ToString($0))", task.dump().c_str());
    return task["taskId"];
}

bool isReady(RequestID& requestId) {
    return results.find(requestId) != results.end();
}

vector<uint8_t> readResult(RequestID& requestId) {
    vector<uint8_t> result;
    requestLock.lock();
    result = std::move(results[requestId]);
    results.erase(requestId);
    requestLock.unlock();
    return std::move(result);
}

void setResult(RequestID& requestId, vector<uint8_t>& data) {
    requestLock.lock();
    results[requestId] = data;
    requestLock.unlock();
}

void endRequest(RequestID& requestId) {
    requestLock.lock();
    results[requestId] = vector<uint8_t>();
    requestLock.unlock();
}

vector<uint8_t> sendGetRequest(string url, uint32_t timeout) {
    vector<uint8_t> content;
    string requestId = sendRequest(url, content, timeout);
    while (!isReady(requestId)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    vector<uint8_t> v = readResult(requestId);
    if (v.size() == 0) {
        throw std::runtime_error("Empty response");
    }
    return std::move(v);
}
vector<uint8_t> sendPostRequest(string url, uint32_t timeout, vector<uint8_t>& content) {
    string requestId = sendRequest(url, content, timeout);
    while (!isReady(requestId)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    vector<uint8_t> v = readResult(requestId);
    if (v.size() == 0) throw std::runtime_error("Empty response");
    return std::move(v);
}