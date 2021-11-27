#pragma once
#include <functional>
#include "transaction.hpp"
#include "block.hpp"
#include "bloomfilter.hpp"
#include "common.hpp"
using namespace std;


int getCurrentBlockCount(string host_url);
string getName(string host_url);
json getBlockData(string host_url, int idx);
json getMiningProblem(string host_url);
json sendTransaction(string host_url, Transaction& t);
json verifyTransaction(string host_url, Transaction& t);
json submitBlock(string host_url, Block& b);
void readRaw(string host_url, int startId, int endId, function<void(Block&)> handler);
void readRawTransactions(string host_url, BloomFilter& bf, function<void(Transaction)> handler);
void readRawTransactionsForBlock(string host_url, int blockId, function<void(Transaction)> handler);
