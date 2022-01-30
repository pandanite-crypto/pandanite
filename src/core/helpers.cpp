
#include "helpers.hpp"
#include "constants.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <climits>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

// windows port for popen/pclose 
#ifdef WIN32
#define popen _popen
#define pclose _pclose
#endif


uint64_t hostToNetworkUint64(uint64_t x) {
    return x;
}

uint32_t hostToNetworkUint32(uint32_t x) {
    return x;
}

uint64_t networkToHostUint64(uint64_t x) {
    return x;
}
uint32_t networkToHostUint32(uint32_t x) {
    return x;
}


using namespace std;

TransactionAmount BMB(double amount) {
    return (TransactionAmount)(amount * DECIMAL_SCALE_FACTOR);
}

uint32_t readNetworkUint32(const char*& buffer) {
    uint32_t x;
    memcpy(&x, buffer, 4);
    buffer += 4;
    return networkToHostUint32(x);
}

uint64_t readNetworkUint64(const char*& buffer) {
    uint64_t x;
    memcpy(&x, buffer, 8);
    buffer += 8;
    return networkToHostUint64(x);
}

SHA256Hash readNetworkSHA256(const char*& buffer) {
    SHA256Hash h;
    memcpy(h.data(), buffer, h.size());
    buffer += h.size();
    return h; 
}

PublicWalletAddress readNetworkPublicWalletAddress(const char*& buffer) {
    PublicWalletAddress w;
    memcpy(w.data(), buffer, w.size());
    buffer += w.size();
    return w;
}

void readNetworkNBytes(const char*& buffer, char* outBuffer, size_t N) {
    memcpy(outBuffer, buffer, N);
    buffer += N;
}

void writeNetworkUint32(char*& buffer, uint32_t x) {
    x = hostToNetworkUint32(x);
    memcpy(buffer, &x, 4);
    buffer+=4;
}

void writeNetworkUint64(char*& buffer, uint64_t x) {
    x = hostToNetworkUint64(x);
    memcpy(buffer, &x, 8);
    buffer+=8;
}

void writeNetworkSHA256(char*& buffer, SHA256Hash& x) {
    memcpy(buffer, x.data(), x.size());
    buffer+=x.size();
}

void writeNetworkPublicWalletAddress(char*& buffer, PublicWalletAddress& x) {
    memcpy(buffer, x.data(), x.size());
    buffer+=x.size();
}

void writeNetworkNBytes(char*& buffer, char const* inputBuffer, size_t N) {
    memcpy(buffer, inputBuffer, N);
    buffer+=N;
}

string randomString(int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    return tmp_s;
}

void writeJsonToFile(json data, string filepath) {
    string dataStr = data.dump();
    ofstream output;
    output.open(filepath);
    output<<dataStr;
    output.close();
}

json readJsonFromFile(string filepath) {
    ifstream input(filepath);
    stringstream buffer;
    buffer << input.rdbuf();
    return json::parse(buffer.str());
}

std::uint64_t getCurrentTime() {
    // HACK: return representable time
    return std::time(0);
}

std::string uint64ToString(const std::uint64_t& t) {
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

std::uint64_t stringToUint64(const std::string& input)
{
    std::istringstream stream(input);
    uint64_t t;
    stream >> t;
    return t;
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

