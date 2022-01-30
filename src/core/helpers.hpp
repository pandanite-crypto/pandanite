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
std::string uint64ToString(const std::uint64_t& t);
std::uint64_t stringToUint64(const std::string& input);
std::string exec(const char* cmd);

uint32_t readNetworkUint32(const char*& buffer);
uint64_t readNetworkUint64(const char*& buffer);
SHA256Hash readNetworkSHA256(const char*& buffer);
PublicWalletAddress readNetworkPublicWalletAddress(const char*& buffer);
void readNetworkNBytes(const char*& buffer, char* outBuffer, size_t N);
void writeNetworkUint32(char*& buffer, uint32_t x);
void writeNetworkUint64(char*& buffer, uint64_t x);
void writeNetworkSHA256(char*& buffer, SHA256Hash& x);
void writeNetworkPublicWalletAddress(char*& buffer, PublicWalletAddress& x);
void writeNetworkNBytes(char*& buffer, char const* inputBuffer, size_t N);
