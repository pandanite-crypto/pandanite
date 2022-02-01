#pragma once
#include <functional>
#include "transaction.hpp"
#include "block.hpp"
#include "common.hpp"
using namespace std;

Bigint getTotalWork(string host_url);
uint32_t getCurrentBlockCount(string host_url);
string getName(string host_url);
json getBlockData(string host_url, int idx);
json getMiningProblem(string host_url);
json sendTransaction(string host_url, Transaction& t);
json sendTransactions(string host_url, vector<Transaction>& transactionList);
json verifyTransaction(string host_url, Transaction& t);
json pingPeer(string host_url, string peer_url, uint64_t time, string version);
json submitBlock(string host_url, Block& b);
void readRawBlocks(string host_url, int startId, int endId, vector<Block>& blocks);
void readRawTransactions(string host_url, vector<Transaction>& transactions);
void readRawHeaders(string host_url, int startId, int endId, vector<BlockHeader>& blockHeaders);