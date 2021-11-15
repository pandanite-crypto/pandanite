
#include "helpers.hpp"
#include "constants.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <climits>

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
