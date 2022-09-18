#pragma once
#include <list>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include "program.hpp"

using namespace std;

class VirtualChain {
    public:
        VirtualChain(Program& program, HostManager& hosts);
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
        std::pair<uint8_t*, size_t> getRaw(uint32_t blockId);
        BlockHeader getBlockHeader(uint32_t blockId);
        TransactionAmount getWalletValue(PublicWalletAddress addr);
        vector<Transaction> getTransactionsForWallet(const PublicWalletAddress& addr) const;
        virtual ProgramID getProgramForWallet(PublicWalletAddress addr);
        uint32_t findBlockForTransaction(Transaction &t);
        uint32_t findBlockForTransactionId(SHA256Hash txid);

        // add or remove blocks to the chain
        ExecutionStatus addBlockSync(Block& block);
        void popBlock();
    protected:
        virtual ExecutionStatus addBlock(Block& block);
        void updateDifficulty();
        ExecutionStatus startChainSync();
        
        // chain state
        bool isSyncing;
        bool shutdown;
        int targetBlockCount;

        Program& program;
        HostManager& hosts;
        std::mutex lock;
        vector<std::thread> syncThread;
        friend void chain_sync(VirtualChain& blockchain);
};
