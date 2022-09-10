#include "../core/crypto.hpp"
#include "../core/user.hpp"
#include "../server/block_store.hpp"
using namespace std;

TEST(test_blockstore_stores_block) {
    BlockStore blocks;
    blocks.init("./test-data/tmpdb");
    Block a;
    a.setId(2);
    User miner;
    User receiver;
    Transaction t = miner.mine();
    a.addTransaction(t);
    // send tiny shares to receiver:
    for(int i = 0; i < 5; i++) {
        Transaction t = miner.send(receiver, 1);
        t.setTimestamp(i);
        a.addTransaction(t);
    }
    ASSERT_EQUAL(blocks.hasBlock(2), false);
    blocks.setBlock(a);
    ASSERT_EQUAL(blocks.hasBlock(2), true);
    Block b = blocks.getBlock(2);
    ASSERT_TRUE(b==a);

    // test we can get transactions for wallets
    PublicWalletAddress to = receiver.getAddress();
    vector<SHA256Hash> txIdsTo = blocks.getTransactionsForWallet(to);
    ASSERT_EQUAL(txIdsTo.size(), 5);
    PublicWalletAddress from = miner.getAddress();
    vector<SHA256Hash> txIdsFrom = blocks.getTransactionsForWallet(from);
    ASSERT_EQUAL(txIdsFrom.size(), 6);

    // test transactions are removed
    blocks.removeBlockWalletTransactions(b);
    txIdsTo = blocks.getTransactionsForWallet(to);
    txIdsFrom = blocks.getTransactionsForWallet(from);
    ASSERT_EQUAL(txIdsTo.size(), 0);
    ASSERT_EQUAL(txIdsFrom.size(), 0);
    blocks.closeDB();
    blocks.deleteDB();
}

TEST(test_blockstore_stores_multiple) {
    BlockStore blocks;
    blocks.init("./test-data/tmpdb");
    User miner;
    User receiver;
    for (int i = 0; i < 30; i++) {
        Block a;
        a.setId(i+1);
        Transaction t = miner.mine();
        a.addTransaction(t);
        // send tiny shares to receiver:
        for(int i = 0; i < 5; i++) {
            a.addTransaction(miner.send(receiver, 1));
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
    blocks.init("./test-data/tmpdb");

    User miner;
    User receiver;
    Block a;
    a.setId(1);
    Transaction t = miner.mine();
    a.addTransaction(t);
    // send tiny shares to receiver:
    for(int i = 0; i < 5; i++) {
        Transaction t = miner.send(receiver, 1);
        a.addTransaction(t);
    }
    blocks.setBlock(a);
    std::pair<uint8_t*,size_t> buffer = blocks.getRawData(a.getId());
    const char* currPtr  = (const char*)buffer.first;
    BlockHeader header = blockHeaderFromBuffer(currPtr);
    currPtr += BLOCKHEADER_BUFFER_SIZE;

    vector<Transaction> transactions;
    for(int i = 0; i < header.numTransactions; i++) {
        TransactionInfo tx = transactionInfoFromBuffer(currPtr);
        currPtr += TRANSACTIONINFO_BUFFER_SIZE;
        transactions.push_back(Transaction(tx));
    }

    Block b = Block(header, transactions);
    ASSERT_TRUE(a==b);
    blocks.closeDB();
    blocks.deleteDB();
}


TEST(test_blockstore_stores_bigint) {
    BlockStore blocks;
    blocks.init("./test-data/tmpdb");

    Bigint a = 2;
    Bigint b = a.pow(10);
    blocks.setTotalWork(b);

    Bigint c = blocks.getTotalWork();

    ASSERT_TRUE(b==c);
    blocks.closeDB();
    blocks.deleteDB();
}
