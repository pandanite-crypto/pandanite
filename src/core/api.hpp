#pragma once
#include "transaction.hpp"
#include "block.hpp"
#include "common.hpp"
using namespace std;

int getCurrentBlockCount(string host_url);
json getBlockData(string host_url, int idx);
json getMiningProblem(string host_url);
json sendTransaction(string host_url, Transaction& t);
json submitBlock(string host_url, Block& b);