#include "../core/block_store.hpp"
#include "../core/crypto.hpp"
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
}