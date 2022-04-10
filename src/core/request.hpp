#include <string>
#include <vector>
#include <thread>
#include "common.hpp"
using namespace std;

vector<uint8_t> sendGetRequest(string url, uint32_t timeout);
vector<uint8_t> sendPostRequest(string url, uint32_t timeout, vector<uint8_t>& content);

typedef std::string RequestID;

class RequestManager {
    public:
        RequestID sendRequest(string& peerId, const char* method, vector<uint8_t> &data);
        bool isReady(RequestID& requestId);
        vector<uint8_t> readResult(RequestID& requestId);
    protected:
        vector<std::thread> fetchThread;
};