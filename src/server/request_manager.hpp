#pragma once
#include <mutex>
#include <vector>
#include <map>
#include <memory>
#include <list>
#include "../core/transaction.hpp"
#include "../core/host_manager.hpp"
#include "../core/common.hpp"
#include "program_store.hpp"
#include "blockchain.hpp"
#include "mempool.hpp"
#include "rate_limiter.hpp"
using namespace std;


class RequestManager {
    public:
        RequestManager(json config);
        ~RequestManager();
        bool acceptRequest(std::string& ip);
        json addTransaction(Transaction& t);
        json getProofOfWork();
        json submitProofOfWork(Block & block);
        json getTransactionQueue();
        json getBlock(uint32_t blockId);
        json getLedger(PublicWalletAddress w);
        json getStats();
        json getTransactionsForWallet(PublicWalletAddress addr);
        json verifyTransaction(Transaction& t);
        json getTransactionStatus(SHA256Hash txid);
        json getPeers();
        json getPeerStats();
        json getProgram(PublicWalletAddress& wallet);
        json setProgram(PublicWalletAddress& wallet, Program& program);
        json getMineStatus(uint32_t blockId);
        json addPeer(string address, uint64_t time, string version, string network);
        BlockHeader getBlockHeader(uint32_t blockId);
        std::pair<uint8_t*, size_t> getRawBlockData(uint32_t blockId);
        std::pair<char*, size_t> getRawTransactionData();
        string getBlockCount();
        string getTotalWork();
        uint64_t getNetworkHashrate();
        void exit();
        void deleteDB();
        void enableRateLimiting(bool enabled);
    protected:
        bool limitRequests;
        HostManager& hosts;
        std::shared_ptr<RateLimiter> rateLimiter;
        std::shared_ptr<BlockChain> blockchain;
        std::shared_ptr<MemPool> mempool;
        std::shared_ptr<HostManager> hosts;
        std::shared_ptr<Program> defaultProgram;
        std::shared_ptr<ProgramStore> programs;
};