#include "../core/blockchain.hpp"
#include "../core/helpers.hpp"
#include "../core/host_manager.hpp"
#include "../core/constants.hpp"
#include "../core/user.hpp"
#include <chrono>
#include <iostream>
#include <thread>
using namespace std;
using namespace std::chrono_literals;

TEST(check_adding_new_node_with_hash) {
    HostManager h;
    BlockChain* blockchain = new BlockChain(h);
    blockchain->resetChain();
    User miner;
    User other;

    // have miner mine the next block
    Transaction fee = miner.mine(2);
    vector<Transaction> transactions;
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    SHA256Hash hash = newBlock.getHash(blockchain->getLastHash());
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    ExecutionStatus status = blockchain->addBlock(newBlock);
    ASSERT_EQUAL(status, SUCCESS);
    delete blockchain;
}

TEST(check_adding_two_nodes_updates_ledger) {
    HostManager h;
    BlockChain* blockchain = new BlockChain(h);
    blockchain->resetChain();
    User miner;

    // have miner mine the next block
    for (int i =2; i <4; i++) {
        Transaction fee = miner.mine(i);
        Block newBlock;
        newBlock.setId(i);
        newBlock.addTransaction(fee);
        SHA256Hash hash = newBlock.getHash(blockchain->getLastHash());
        SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
        newBlock.setNonce(solution);
        ExecutionStatus status = blockchain->addBlock(newBlock);
        ASSERT_EQUAL(status, SUCCESS);
    }
    Ledger& ledger = blockchain->getLedger();
    double total = ledger.getWalletValue(miner.getAddress());
    ASSERT_EQUAL(total, BMB(100.0));
    delete blockchain;
}

TEST(check_sending_transaction_updates_ledger) {
    HostManager h;
    BlockChain* blockchain = new BlockChain(h);
    blockchain->resetChain();
    User miner;
    User other;

    // have miner mine the next block
    for (int i =2; i <4; i++) {
        Transaction fee = miner.mine(i);
        Block newBlock;
        newBlock.setId(i);
        newBlock.addTransaction(fee);
        if (i==3) {
            Transaction t = miner.send(other, BMB(20.0),i);
            newBlock.addTransaction(t);
        }
        
        SHA256Hash hash = newBlock.getHash(blockchain->getLastHash());
        
        SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
        newBlock.setNonce(solution);
        ExecutionStatus status = blockchain->addBlock(newBlock);
        ASSERT_EQUAL(status, SUCCESS);
    }
    Ledger& ledger = blockchain->getLedger();
    double minerTotal = ledger.getWalletValue(miner.getAddress());
    double otherTotal = ledger.getWalletValue(other.getAddress());

    ASSERT_EQUAL(minerTotal, BMB(80.0));
    ASSERT_EQUAL(otherTotal, BMB(20.0));
    delete blockchain;
}