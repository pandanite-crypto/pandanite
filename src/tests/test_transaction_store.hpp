#include "../core/transaction.hpp"
#include "../server/tx_store.hpp"
using namespace std;

TEST(test_txdb_stores_transaction) {
    TransactionStore txdb;
    User miner;
    User other;
    txdb.init("./test-data/tmpdb");
    Transaction t = miner.mine(1);
    ASSERT_EQUAL(txdb.hasTransaction(t), false);
    txdb.insertTransaction(t);
    ASSERT_EQUAL(txdb.hasTransaction(t), true);
    txdb.removeTransaction(t);
    ASSERT_EQUAL(txdb.hasTransaction(t), false);

    Transaction t2 = miner.send(other, 1,1);
    ASSERT_EQUAL(txdb.hasTransaction(t2), false);
    txdb.insertTransaction(t2);
    ASSERT_EQUAL(txdb.hasTransaction(t2), true);
    txdb.removeTransaction(t2);
    ASSERT_EQUAL(txdb.hasTransaction(t2), false);
}