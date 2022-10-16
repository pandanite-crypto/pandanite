#pragma once
#include <mutex>
#include <vector>
#include <map>
#include <memory>
#include <list>
#include "../core/transaction.hpp"
#include "../shared/host_manager.hpp"
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
        json addTransaction(Transaction& t, ProgramID program = NULL_SHA256_HASH);
        json getProofOfWork(ProgramID program = NULL_SHA256_HASH);
        json submitProofOfWork(Block & block, ProgramID program = NULL_SHA256_HASH);
        json getTransactionQueue();
        json getBlock(uint32_t blockId, ProgramID program = NULL_SHA256_HASH);
        json getLedger(PublicWalletAddress w, ProgramID program = NULL_SHA256_HASH);
        json getStats();
        json getTransactionsForWallet(PublicWalletAddress addr, ProgramID program = NULL_SHA256_HASH);
        json verifyTransaction(Transaction& t, ProgramID program = NULL_SHA256_HASH);
        json getTransactionStatus(SHA256Hash txid, ProgramID program = NULL_SHA256_HASH);
        json getPeers(ProgramID program = NULL_SHA256_HASH);
        json getPeerStats();
        json getConfig();
        json getInfo(json args, ProgramID program = NULL_SHA256_HASH);
        json getProgram(ProgramID programId);
        json setProgram(std::shared_ptr<Program> program);
        json getMineStatus(uint32_t blockId, ProgramID program = NULL_SHA256_HASH);
        json addPeer(string address, uint64_t time, string version, string network);
        BlockHeader getBlockHeader(uint32_t blockId, ProgramID program = NULL_SHA256_HASH);
        std::pair<uint8_t*, size_t> getRawBlockData(uint32_t blockId, ProgramID program = NULL_SHA256_HASH);
        std::pair<char*, size_t> getRawTransactionData();
        string getHostAddress();
        string getBlockCount(ProgramID program = NULL_SHA256_HASH);
        string getTotalWork(ProgramID program = NULL_SHA256_HASH);
        uint64_t getNetworkHashrate();
        std::shared_ptr<VirtualChain> getChain(ProgramID program);
        std::shared_ptr<BlockChain> getMainChain();
        void exit();
        void deleteDB();
        void enableRateLimiting(bool enabled);
    protected:
        json config;
        bool limitRequests;
        map<ProgramID, std::shared_ptr<VirtualChain>> subchains;
        std::shared_ptr<RateLimiter> rateLimiter;
        std::shared_ptr<MemPool> mempool;
        std::shared_ptr<HostManager> hosts;
        std::shared_ptr<Program> defaultProgram;
        std::shared_ptr<ProgramStore> programs;
};