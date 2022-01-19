#pragma once
#include <string>
#include "common.hpp"
#include <ctime>
using namespace std;

string randomString(int length);
void writeJsonToFile(json data, string filepath);
json readJsonFromFile(string filepath);
TransactionAmount BMB(double amount);
std::uint64_t getCurrentTime();
std::string timeToString(const std::uint64_t& t);
std::uint64_t stringToTime(const std::string& input);
std::string exec(const char* cmd);
