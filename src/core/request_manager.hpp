#pragma once
#include "blockchain.hpp"
#include "transaction.hpp"
#include "host_manager.hpp"
#include "common.hpp"
#include <vector>
#include <map>
#include <list>
using namespace std;


class RequestManager {
    public:
        RequestManager(HostManager& hosts);
        json addTransaction(json data);
        json getProofOfWork();
        json submitProofOfWork(json data);
        json getBlock(int index);
        json getLedger(PublicWalletAddress w);
        json getStats();
        json verifyTransaction(json data);
        string getBlockCount();
        BlockChain* blockchain;
    protected:
        HostManager& hosts;
        size_t getPendingTransactionSize(int block);
        map<int,list<Transaction>> transactionQueue;
};