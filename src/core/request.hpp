#pragma once
#include <string>
#include <vector>
#include <thread>
#include "common.hpp"
using namespace std;

vector<uint8_t> sendGetRequest(string url, uint32_t timeout);
vector<uint8_t> sendPostRequest(string url, uint32_t timeout, vector<uint8_t>& content);

#define REQUEST_ID_LENGTH 4
typedef std::string RequestID;

void setResult(RequestID& requestId, vector<uint8_t>& data);
void endRequest(RequestID& requestId);
