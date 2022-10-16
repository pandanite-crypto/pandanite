#pragma once
#include <list>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include "program.hpp"
#include "mempool.hpp"

using namespace std;

class VirtualChain {
    public:
        VirtualChain(std::shared_ptr<Program> program, HostManager& hosts);
        ~VirtualChain();

        // init, reset or start synchronization
        void initChain();
        void resetChain();
        void sync();

        // look up information regarding the chain
        Block getBlock(uint32_t blockId);
        Bigint getTotalWork();
        int getDifficulty();
        uint32_t getBlockCount();
        TransactionAmount getCurrentMiningFee();
        SHA256Hash getLastHash();
        void setMemPool(std::shared_ptr<MemPool> memPool);
        json getInfo(json args);
        std::pair<uint8_t*, size_t> getRaw(uint32_t blockId);
        BlockHeader getBlockHeader(uint32_t blockId);
        TransactionAmount getWalletValue(PublicWalletAddress addr);
        vector<Transaction> getTransactionsForWallet(const PublicWalletAddress& addr) const;
        std::shared_ptr<Program> getProgram();
        virtual ProgramID getProgramForWallet(PublicWalletAddress addr);
        uint32_t findBlockForTransaction(Transaction &t);
        uint32_t findBlockForTransactionId(SHA256Hash txid);

        // add or remove blocks to the chain
        ExecutionStatus addBlockSync(Block& block);
        void popBlock();

    protected:
        std::shared_ptr<MemPool> memPool;
        virtual ExecutionStatus addBlock(Block& block);
        void updateDifficulty();
        ExecutionStatus startChainSync();
        
        // chain state
        bool isSyncing;
        bool shutdown;
        int targetBlockCount;

        std::shared_ptr<Program> program;
        HostManager& hosts;
        std::mutex lock;
        vector<std::thread> syncThread;
        friend void chain_sync(VirtualChain& blockchain);
};
