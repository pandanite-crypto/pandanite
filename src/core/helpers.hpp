#pragma once
#include <string>
#include "common.hpp"
#include <ctime>
using namespace std;

string randomString(int length);
void writeJsonToFile(json data, string filepath);
json readJsonFromFile(string filepath);
TransactionAmount BMB(double amount);
std::time_t getCurrentTime();
std::string timeToString(const std::time_t& t);
std::time_t stringToTime(const std::string& input);