
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

using namespace std;

TransactionAmount BMB(double amount) {
    return (TransactionAmount)(amount * DECIMAL_SCALE_FACTOR);
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

