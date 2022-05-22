#include "../core/helpers.hpp"
#include "../core/merkle_tree.hpp"
#include "../core/host_manager.hpp"
#include "../core/constants.hpp"
#include "../core/user.hpp"
#include "../server/blockchain.hpp"
#include <chrono>
#include <iostream>
#include <thread>
using namespace std;
using namespace std::chrono_literals;

string ledger = "./test-data/ledger";
string blocks = "./test-data/blocks";
string txdb = "./test-data/txdb";

void addMerkleHashToBlock(Block& block) {
    // compute merkle tree and verify root matches;
    MerkleTree m;
    m.setItems(block.getTransactions());
    SHA256Hash computedRoot = m.getRootHash();
    block.setMerkleRoot(m.getRootHash());
}

TEST(check_adding_new_node_with_hash) {
    HostManager h;
    BlockChain* blockchain = new BlockChain(h, ledger, blocks, txdb);
    User miner;
    User other;
    Transaction fee = miner.mine();
    vector<Transaction> transactions;
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    addMerkleHashToBlock(newBlock);
    newBlock.setLastBlockHash(blockchain->getLastHash());
    SHA256Hash hash = newBlock.getHash();
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    ExecutionStatus status = blockchain->addBlock(newBlock);
    ASSERT_EQUAL(status, SUCCESS);
    blockchain->deleteDB();
    delete blockchain;
}

TEST(check_popping_block) {
    HostManager h;
    BlockChain* blockchain = new BlockChain(h, ledger, blocks, txdb);
    User miner;
    User other;
    // have miner mine the next block
    Transaction fee = miner.mine();
    vector<Transaction> transactions;
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    addMerkleHashToBlock(newBlock);
    newBlock.setLastBlockHash(blockchain->getLastHash());
    SHA256Hash hash = newBlock.getHash();
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    ExecutionStatus status = blockchain->addBlock(newBlock);
    ASSERT_EQUAL(blockchain->getLedger().getWalletValue(miner.getAddress()), BMB(50));
    blockchain->popBlock();
    bool threw = false;
    // wallet should no longer exist:
    try {
        blockchain->getLedger().getWalletValue(miner.getAddress());
    } catch (...) {
        threw = true;
    }
    ASSERT_EQUAL(threw, true);
    blockchain->deleteDB();
    delete blockchain;
}


TEST(check_adding_wrong_lastblock_hash_fails) {
    HostManager h;
    BlockChain* blockchain = new BlockChain(h, ledger, blocks, txdb);
    User miner;
    User other;
    // have miner mine the next block
    Transaction fee = miner.mine();
    vector<Transaction> transactions;
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    addMerkleHashToBlock(newBlock);
    newBlock.setLastBlockHash(NULL_SHA256_HASH);
    SHA256Hash hash = newBlock.getHash();
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    ExecutionStatus status = blockchain->addBlock(newBlock);
    ASSERT_EQUAL(status, INVALID_LASTBLOCK_HASH);
    blockchain->deleteDB();
    delete blockchain;
}

TEST(check_adding_two_nodes_updates_ledger) {
    HostManager h;
    BlockChain* blockchain = new BlockChain(h, ledger, blocks, txdb);
    User miner;

    // have miner mine the next block
    for (int i =2; i <4; i++) {
        Transaction fee = miner.mine();
        Block newBlock;
        newBlock.setId(i);
        newBlock.addTransaction(fee);
        addMerkleHashToBlock(newBlock);
        newBlock.setLastBlockHash(blockchain->getLastHash());
        SHA256Hash hash = newBlock.getHash();
        SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
        newBlock.setNonce(solution);
        ExecutionStatus status = blockchain->addBlock(newBlock);
        ASSERT_EQUAL(status, SUCCESS);
    }
    Ledger& ledger = blockchain->getLedger();
    double total = ledger.getWalletValue(miner.getAddress());
    ASSERT_EQUAL(total, BMB(100.0));
    blockchain->deleteDB();
    delete blockchain;
}

TEST(check_sending_transaction_updates_ledger) {
    HostManager h;
    BlockChain* blockchain = new BlockChain(h, ledger, blocks, txdb);
    User miner;
    User other;

    // have miner mine the next block
    for (int i =2; i <4; i++) {
        Transaction fee = miner.mine();
        Block newBlock;
        newBlock.setId(i);
        newBlock.addTransaction(fee);
        if (i==3) {
            Transaction t = miner.send(other, BMB(20.0));
            newBlock.addTransaction(t);
        }
        addMerkleHashToBlock(newBlock);
        newBlock.setLastBlockHash(blockchain->getLastHash());
        SHA256Hash hash = newBlock.getHash();
        SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
        newBlock.setNonce(solution);
        ExecutionStatus status = blockchain->addBlock(newBlock);
        ASSERT_EQUAL(status, SUCCESS);
    }
    Ledger& ledger = blockchain->getLedger();
    double minerTotal = ledger.getWalletValue(miner.getAddress());
    double otherTotal = ledger.getWalletValue(other.getAddress());
    Bigint totalWork = blockchain->getTotalWork();
    Bigint base = 2;
    Bigint work = base.pow(MIN_DIFFICULTY);
    Bigint num = 3;
    Bigint total = num * work;
    ASSERT_EQUAL(totalWork, total);
    ASSERT_EQUAL(minerTotal, BMB(80.0));
    ASSERT_EQUAL(otherTotal, BMB(20.0));
    blockchain->deleteDB();
    delete blockchain;
}


TEST(check_duplicate_tx_fails) {
    HostManager h;
    BlockChain* blockchain = new BlockChain(h, ledger, blocks, txdb);
    User miner;
    User other;

    Transaction t = miner.send(other, BMB(20.0));
    // have miner mine the next block
    for (int i =2; i <=4; i++) {
        Transaction fee = miner.mine();
        Block newBlock;
        newBlock.setId(i);
        newBlock.addTransaction(fee);
        
        if (i==3 || i==4) {
            newBlock.addTransaction(t);
        }
        addMerkleHashToBlock(newBlock);
        newBlock.setLastBlockHash(blockchain->getLastHash());
        SHA256Hash hash = newBlock.getHash();
        SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
        newBlock.setNonce(solution);
        ExecutionStatus status = blockchain->addBlock(newBlock);
        if (i==4) { 
            ASSERT_EQUAL(status, EXPIRED_TRANSACTION);
        }else {
            ASSERT_EQUAL(status, SUCCESS);
        }
    }
    blockchain->deleteDB();
    delete blockchain;
}