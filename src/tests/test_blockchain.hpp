#include "../core/blockchain.hpp"
#include "../core/helpers.hpp"
#include "../core/constants.hpp"
#include <chrono>
#include <iostream>
#include <thread>
using namespace std;
using namespace std::chrono_literals;

TEST(check_adding_new_node_with_hash) {
    BlockChain* blockchain = new BlockChain();
    blockchain->resetChain();
    User miner;
    User other;

    // have miner mine the next block
    Transaction fee = miner.mine(1);
    vector<Transaction> transactions;
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    SHA256Hash hash = newBlock.getHash(blockchain->getLastHash());
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    ExecutionStatus status = blockchain->addBlock(newBlock);
    ASSERT_EQUAL(status, SUCCESS);
}

TEST(check_adding_two_nodes_updates_ledger) {
    BlockChain* blockchain = new BlockChain();
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
    LedgerState &ledger = blockchain->getLedger();
    double total = ledger[miner.getAddress()];
    ASSERT_EQUAL(total, 100.0);
}

TEST(check_sending_transaction_updates_ledger) {
    BlockChain* blockchain = new BlockChain();
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
            Transaction t = miner.send(other, 20.0,i);
            newBlock.addTransaction(t);
        }
        
        SHA256Hash hash = newBlock.getHash(blockchain->getLastHash());
        
        SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
        newBlock.setNonce(solution);
        ExecutionStatus status = blockchain->addBlock(newBlock);
        ASSERT_EQUAL(status, SUCCESS);
    }
    LedgerState &ledger = blockchain->getLedger();
    double minerTotal = ledger[miner.getAddress()];
    double otherTotal = ledger[other.getAddress()];

    ASSERT_EQUAL(minerTotal, 80.0);
    ASSERT_EQUAL(otherTotal, 20.0);
}