#pragma once
#include "blockchain.hpp"
#include "transaction.hpp"
#include "host_manager.hpp"
#include "mempool.hpp"
#include "common.hpp"
#include <mutex>
#include <vector>
#include <map>
#include <list>
using namespace std;


class RequestManager {
    public:
        RequestManager(HostManager& hosts);
        json addTransaction(Transaction& t);
        json getProofOfWork();
        json submitProofOfWork(Block & block);
        json getBlock(int index);
        json getLedger(PublicWalletAddress w);
        json getStats();
        json verifyTransaction(Transaction& t);
        void setTaxRate(double rate);
        std::pair<char*, size_t> getRawBlockData(int index);
        std::pair<char*, size_t> getRawTransactionData();
        string getBlockCount();
        void deleteDB();
    protected:
        HostManager& hosts;
        BlockChain* blockchain;
        MemPool* mempool;
        std::mutex transactionsLock;
        size_t getPendingTransactionSize(int block);
        map<int,list<Transaction>> transactionQueue;
};