#include "../core/helpers.hpp"
#include "../core/merkle_tree.hpp"
#include "../shared/host_manager.hpp"
#include "../core/constants.hpp"
#include "../shared/user.hpp"
#include "../server/blockchain.hpp"
#include "../server/program.hpp"
#include <chrono>
#include <iostream>
#include <thread>
using namespace std;
using namespace std::chrono_literals;

void addMerkleHashToBlock(Block& block) {
    // compute merkle tree and verify root matches;
    MerkleTree m;
    m.setItems(block.getTransactions());
    SHA256Hash computedRoot = m.getRootHash();
    block.setMerkleRoot(m.getRootHash());
}

TEST(check_adding_new_node_with_hash) {
    HostManager hosts;
    Program defaultProgram;
    BlockChain* blockchain = new BlockChain(defaultProgram, hosts);
    User miner;
    User other;
    Transaction fee = miner.mine();
    vector<Transaction> transactions;
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    addMerkleHashToBlock(newBlock);
    newBlock.setLastBlockHash(blockchain->getLastHash());
    newBlock.setDifficulty(blockchain->getDifficulty());
    SHA256Hash hash = newBlock.getHash();
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    ExecutionStatus status = blockchain->addBlockSync(newBlock);
    ASSERT_EQUAL(status, SUCCESS);
    defaultProgram.deleteData();
}

TEST(check_popping_block) {
    HostManager hosts;
    Program defaultProgram;
    BlockChain* blockchain = new BlockChain(defaultProgram, hosts);
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
    newBlock.setDifficulty(blockchain->getDifficulty());
    SHA256Hash hash = newBlock.getHash();
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    ExecutionStatus status = blockchain->addBlockSync(newBlock);
    ASSERT_EQUAL(blockchain->getWalletValue(miner.getAddress()), BMB(50));
    blockchain->popBlock();
    bool threw = false;
    // wallet should no longer exist:
    try {
        blockchain->getWalletValue(miner.getAddress());
    } catch (...) {
        threw = true;
    }
    ASSERT_EQUAL(threw, true);
    defaultProgram.deleteData();
}


TEST(check_adding_wrong_lastblock_hash_fails) {
    HostManager hosts;
    Program defaultProgram;
    BlockChain* blockchain = new BlockChain(defaultProgram, hosts);
    User miner;
    User other;
    // have miner mine the next block
    Transaction fee = miner.mine();
    vector<Transaction> transactions;
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    newBlock.setDifficulty(blockchain->getDifficulty());
    addMerkleHashToBlock(newBlock);
    newBlock.setLastBlockHash(NULL_SHA256_HASH);
    SHA256Hash hash = newBlock.getHash();
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    ExecutionStatus status = blockchain->addBlockSync(newBlock);
    ASSERT_EQUAL(status, INVALID_LASTBLOCK_HASH);
    defaultProgram.deleteData();
}

TEST(check_adding_program_locks_ledger) {
    HostManager hosts;
    Program defaultProgram;
    BlockChain* blockchain = new BlockChain(defaultProgram, hosts);
    User miner;

    // have miner mine and add chain lock 
    Transaction fee = miner.mine();
    ProgramID progId = NULL_SHA256_HASH;
    progId[0] = 0xf;
    Transaction chainLock = Transaction(miner.getAddress(), miner.getPublicKey(), 0, progId);
    miner.signTransaction(chainLock);
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    newBlock.addTransaction(chainLock);
    newBlock.setDifficulty(blockchain->getDifficulty());
    addMerkleHashToBlock(newBlock);
    newBlock.setLastBlockHash(blockchain->getLastHash());
    SHA256Hash hash = newBlock.getHash();
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    ExecutionStatus status = blockchain->addBlockSync(newBlock);
    ASSERT_EQUAL(status, SUCCESS);

    // now attempt to send from locked address
    fee = miner.mine();
    Block invalidBlock;
    invalidBlock.setId(3);
    invalidBlock.addTransaction(fee);
    // transaction that should fail (sending from locked wallet)
    User other;
    Transaction f = miner.send(other, 1);
    invalidBlock.addTransaction(f);
    invalidBlock.setDifficulty(blockchain->getDifficulty());
    addMerkleHashToBlock(invalidBlock);
    invalidBlock.setLastBlockHash(blockchain->getLastHash());
    hash = invalidBlock.getHash();
    solution = mineHash(hash, invalidBlock.getDifficulty());
    invalidBlock.setNonce(solution);
    status = blockchain->addBlockSync(invalidBlock);
    ASSERT_EQUAL(status, WALLET_LOCKED);
    ASSERT_EQUAL(progId, blockchain->getProgramForWallet(miner.getAddress()));
    // now pop off the block with the chain lock
    blockchain->popBlock();
    // adding should now work:
    Block validBlock;
    validBlock.setId(2);
    validBlock.addTransaction(fee);
    validBlock.setDifficulty(blockchain->getDifficulty());
    validBlock.setLastBlockHash(blockchain->getLastHash());
    addMerkleHashToBlock(validBlock);
    hash = validBlock.getHash();
    solution = mineHash(hash, blockchain->getDifficulty());
    validBlock.setNonce(solution);
    status = blockchain->addBlockSync(validBlock);
    ASSERT_EQUAL(status, SUCCESS);
    defaultProgram.deleteData();
}



TEST(check_adding_two_nodes_updates_ledger) {
    HostManager hosts;
    Program defaultProgram;
    BlockChain* blockchain = new BlockChain(defaultProgram, hosts);
    User miner;

    // have miner mine the next block
    for (int i =2; i <4; i++) {
        Transaction fee = miner.mine();
        Block newBlock;
        newBlock.setId(i);
        newBlock.addTransaction(fee);
        newBlock.setDifficulty(blockchain->getDifficulty());
        addMerkleHashToBlock(newBlock);
        newBlock.setLastBlockHash(blockchain->getLastHash());
        SHA256Hash hash = newBlock.getHash();
        SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
        newBlock.setNonce(solution);
        ExecutionStatus status = blockchain->addBlockSync(newBlock);
        ASSERT_EQUAL(status, SUCCESS);
    }
    double total = blockchain->getWalletValue(miner.getAddress());
    ASSERT_EQUAL(total, BMB(100.0));
    defaultProgram.deleteData();
}

TEST(check_sending_transaction_updates_ledger) {
    HostManager hosts;
    Program defaultProgram;
    BlockChain* blockchain = new BlockChain(defaultProgram, hosts);
    User miner;
    User other;

    // have miner mine the next block
    for (int i =2; i <4; i++) {
        Transaction fee = miner.mine();
        Block newBlock;
        newBlock.setId(i);
        newBlock.addTransaction(fee);
        newBlock.setDifficulty(blockchain->getDifficulty());
        if (i==3) {
            Transaction t = miner.send(other, BMB(20.0));
            newBlock.addTransaction(t);
        }
        addMerkleHashToBlock(newBlock);
        newBlock.setLastBlockHash(blockchain->getLastHash());
        SHA256Hash hash = newBlock.getHash();
        SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
        newBlock.setNonce(solution);
        ExecutionStatus status = blockchain->addBlockSync(newBlock);
        ASSERT_EQUAL(status, SUCCESS);
    }
    double minerTotal = blockchain->getWalletValue(miner.getAddress());
    double otherTotal = blockchain->getWalletValue(other.getAddress());
    Bigint totalWork = blockchain->getTotalWork();
    Bigint base = 2;
    Bigint work = base.pow(blockchain->getDifficulty());
    Bigint num = 3;
    Bigint total = num * work;
    ASSERT_EQUAL(totalWork, total);
    ASSERT_EQUAL(minerTotal, BMB(80.0));
    ASSERT_EQUAL(otherTotal, BMB(20.0));
    defaultProgram.deleteData();
}


TEST(check_duplicate_tx_fails) {
    HostManager hosts;
    Program defaultProgram;
    BlockChain* blockchain = new BlockChain(defaultProgram, hosts);
    User miner;
    User other;

    Transaction t = miner.send(other, BMB(20.0));
    // have miner mine the next block
    for (int i =2; i <=4; i++) {
        Transaction fee = miner.mine();
        Block newBlock;
        newBlock.setId(i);
        newBlock.setDifficulty(blockchain->getDifficulty());
        newBlock.addTransaction(fee);
        
        if (i==3 || i==4) {
            newBlock.addTransaction(t);
        }
        addMerkleHashToBlock(newBlock);
        newBlock.setLastBlockHash(blockchain->getLastHash());
        SHA256Hash hash = newBlock.getHash();
        SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
        newBlock.setNonce(solution);
        ExecutionStatus status = blockchain->addBlockSync(newBlock);
        if (i==4) { 
            ASSERT_EQUAL(status, EXPIRED_TRANSACTION);
        }else {
            ASSERT_EQUAL(status, SUCCESS);
        }
    }
    defaultProgram.deleteData();
}