#include "../core/crypto.hpp"
#include "../core/user.hpp"
#include "../server/block_store.hpp"
using namespace std;

TEST(test_blockstore_stores_block) {
    BlockStore blocks;
    blocks.init("./data/tmpdb");
    Block a;
    a.setId(2);
    User miner;
    User receiver;
    Transaction t = miner.mine(a.getId());
    a.addTransaction(t);
    // send tiny shares to receiver:
    for(int i = 0; i < 5; i++) {
        a.addTransaction(miner.send(receiver, 1, a.getId()));
    }
    ASSERT_EQUAL(blocks.hasBlock(2), false);
    blocks.setBlock(a);
    ASSERT_EQUAL(blocks.hasBlock(2), true);
    Block b = blocks.getBlock(2);
    ASSERT_TRUE(b==a);
    blocks.closeDB();
    blocks.deleteDB();
}

TEST(test_blockstore_stores_multiple) {
    BlockStore blocks;
    blocks.init("./data/tmpdb");
    

    User miner;
    User receiver;
    for (int i = 0; i < 30; i++) {
        Block a;
        a.setId(i+1);
        Transaction t = miner.mine(a.getId());
        a.addTransaction(t);
        // send tiny shares to receiver:
        for(int i = 0; i < 5; i++) {
            a.addTransaction(miner.send(receiver, 1, a.getId()));
        }
        ASSERT_EQUAL(blocks.hasBlock(i+1), false);
        blocks.setBlock(a);
        ASSERT_EQUAL(blocks.hasBlock(i+1), true);
        Block b = blocks.getBlock(i+1);
        ASSERT_TRUE(b==a);
    }
    blocks.setBlockCount(30);
    blocks.setTotalWork(30*MIN_DIFFICULTY);
    ASSERT_TRUE(blocks.getBlockCount() == 30);
    ASSERT_TRUE(blocks.getTotalWork() == 30*MIN_DIFFICULTY);
    blocks.closeDB();
    blocks.deleteDB();
}

TEST(test_blockstore_returns_valid_raw_data) {
    BlockStore blocks;
    blocks.init("./data/tmpdb");

    User miner;
    User receiver;
    Block a;
    a.setId(1);
    Transaction t = miner.mine(a.getId());
    a.addTransaction(t);
    // send tiny shares to receiver:
    for(int i = 0; i < 5; i++) {
        Transaction t = miner.send(receiver, 1, a.getId());
        a.addTransaction(t);
    }
    blocks.setBlock(a);
    std::pair<uint8_t*,size_t> buffer = blocks.getRawData(a.getId());
    Block b = Block(buffer);
    ASSERT_TRUE(a==b);
    blocks.closeDB();
    blocks.deleteDB();
}