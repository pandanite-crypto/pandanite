#pragma once
#include <mutex>
#include <vector>
#include <map>
#include <list>
#include "../core/transaction.hpp"
#include "../core/host_manager.hpp"
#include "../core/bloomfilter.hpp"
#include "../core/common.hpp"
#include "blockchain.hpp"
#include "mempool.hpp"
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
        json getPeers();
        json addPeer(string host);
        std::pair<uint8_t*, size_t> getBlockHeaders(uint32_t start, uint32_t end);
        std::pair<uint8_t*, size_t> getRawBlockData(uint32_t index);
        std::pair<char*, size_t> getRawTransactionData();
        string getBlockCount();
        string getTotalWork();
        void deleteDB();
    protected:
        HostManager& hosts;
        BlockChain* blockchain;
        MemPool* mempool;
};