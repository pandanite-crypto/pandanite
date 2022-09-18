#pragma once
#include "virtual_chain.hpp"
#include "mempool.hpp"
using namespace std;


class BlockChain : public VirtualChain {
    public:
        void setMemPool(MemPool * memPool);
        ExecutionStatus addBlock(Block& block);
        ProgramID getProgramForWallet(PublicWalletAddress addr);
        ExecutionStatus verifyTransaction(const Transaction& t);
    protected:
        MemPool * memPool;
};
