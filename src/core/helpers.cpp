
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

using namespace std;

TransactionAmount BMB(double amount) {
    return amount * DECIMAL_SCALE_FACTOR;
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

std::time_t getCurrentTime() {
    // HACK: return representable time
    return stringToTime(timeToString(std::time(0)));
}

std::string timeToString(const std::time_t& t) {
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

std::time_t stringToTime(const std::string& input)
{
    std::istringstream stream(input);
    time_t t;
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


std::string curlGet(string url) {
    auto curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        
        std::string response_string;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl = NULL;
        return response_string;
    }
}