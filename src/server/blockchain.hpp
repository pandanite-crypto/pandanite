#pragma once
#include "virtual_chain.hpp"
#include "mempool.hpp"
#include <map>
using namespace std;


class BlockChain : public VirtualChain {
    public:
        BlockChain(std::shared_ptr<Program> program, HostManager& hosts);
        ExecutionStatus addBlock(Block& block);
        ProgramID getProgramForWallet(PublicWalletAddress addr);
        ExecutionStatus verifyTransaction(const Transaction& t);
        map<string, uint64_t> getHeaderChainStats() const;
};
