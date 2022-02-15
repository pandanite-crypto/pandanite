#pragma once
#include <mutex>
#include <vector>
#include <map>
#include <list>
#include "../core/transaction.hpp"
#include "../core/host_manager.hpp"
#include "../core/common.hpp"
#include "blockchain.hpp"
#include "mempool.hpp"
#include "rate_limiter.hpp"
using namespace std;


class RequestManager {
    public:
        RequestManager(HostManager& hosts, string ledgerPath="", string blockPath="", string txdbPath="");
        ~RequestManager();
        bool acceptRequest(std::string& ip);
        json addTransaction(Transaction& t);
        json getProofOfWork();
        json submitProofOfWork(Block & block);
        json getTransactionQueue();
        json getBlock(uint32_t blockId);
        json getLedger(PublicWalletAddress w);
        json getStats();
        json verifyTransaction(Transaction& t);
        json getPeers();
        json getMineStatus(uint32_t blockId);
        json addPeer(string address, uint64_t time, string version);
        BlockHeader getBlockHeader(uint32_t blockId);
        std::pair<uint8_t*, size_t> getRawBlockData(uint32_t blockId);
        std::pair<char*, size_t> getRawTransactionData();
        string getBlockCount();
        string getTotalWork();
        void deleteDB();
        void enableRateLimiting(bool enabled);
    protected:
        bool limitRequests;
        HostManager& hosts;
        RateLimiter* rateLimiter;
        BlockChain* blockchain;
        MemPool* mempool;
};