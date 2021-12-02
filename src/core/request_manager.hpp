#pragma once
#include "blockchain.hpp"
#include "transaction.hpp"
#include "host_manager.hpp"
#include "mempool.hpp"
#include "common.hpp"
#include "bloomfilter.hpp"
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
        json getBlock(uint32_t index);
        json getLedger(PublicWalletAddress w);
        json getStats();
        json verifyTransaction(Transaction& t);
        std::pair<uint8_t*, size_t> getBlockHeaders(uint32_t start, uint32_t end);
        std::pair<uint8_t*, size_t> getRawBlockData(uint32_t index);
        std::pair<char*, size_t> getRawTransactionData();
        std::pair<char*, size_t> getRawTransactionData(BloomFilter& seen);
        std::pair<char*, size_t> getRawTransactionDataForBlock(uint32_t blockId);
        string getBlockCount();
        string getTotalWork();
        void deleteDB();
    protected:
        HostManager& hosts;
        BlockChain* blockchain;
        MemPool* mempool;
        std::mutex transactionsLock;
        size_t getPendingTransactionSize(int block);
        map<int,list<Transaction>> transactionQueue;
};