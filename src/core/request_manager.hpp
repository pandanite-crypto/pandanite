#pragma once
#include "blockchain.hpp"
#include "transaction.hpp"
#include "common.hpp"
#include <vector>
#include <map>
#include <list>
using namespace std;


class RequestManager {
    public:
        RequestManager(vector<string> hosts);
        json addTransaction(json data);
        json getProofOfWork();
        json submitProofOfWork(json data);
        json getBlock(int index);
        json getLedger(PublicWalletAddress w);
        json getStats();
        string getBlockCount();
        BlockChain* blockchain;
    protected:
        size_t getPendingTransactionSize(int block);
        map<int,list<Transaction>> transactionQueue;
};