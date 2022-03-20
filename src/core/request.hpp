#include <string>
#include <vector>
#include "common.hpp"
using namespace std;

vector<uint8_t> sendGetRequest(string url, uint32_t timeout);
vector<uint8_t> sendPostRequest(string url, uint32_t timeout, vector<uint8_t>& content);
