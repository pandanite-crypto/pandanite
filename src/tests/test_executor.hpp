#include "../core/user.hpp"
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include "../core/constants.hpp"
#include "../core/executor.hpp"
#include "../core/helpers.hpp"
#include "../core/common.hpp"
#include <iostream>
using namespace std;

TEST(checks_invalid_mining_fee) {
    Block b;
    User miner;

    // add incorrect mining fee to block
    Transaction t = miner.mine(1);
    t.setAmount(100.0);
    b.addTransaction(t);
    Ledger ledger;
    ledger.init("./data/tmpdb");
    LedgerState deltas;
    ExecutionStatus status;
    status = Executor::ExecuteBlock(b, ledger, deltas);
    ASSERT_EQUAL(status, INCORRECT_MINING_FEE);     
    ledger.closeDB();
    ledger.deleteDB();
}

TEST(checks_duplicate_mining_fee) {
    Block b;
    Ledger ledger;
    ledger.init("./data/tmpdb");
    LedgerState deltas;
    ExecutionStatus status;
    User miner;
    // add mining transaction twice
    Transaction t1 = miner.mine(1);
    Transaction t2 = miner.mine(1);
    b.addTransaction(t1);
    b.addTransaction(t2);

    status = Executor::ExecuteBlock(b, ledger, deltas);
    ASSERT_EQUAL(status, EXTRA_MINING_FEE);    
    ledger.closeDB();
    ledger.deleteDB(); 
}

TEST(checks_missing_mining_fee) {
    Block b;
    Ledger ledger;
    ledger.init("./data/tmpdb");
    LedgerState deltas;
    ExecutionStatus status;
    status = Executor::ExecuteBlock(b, ledger, deltas);
    ASSERT_EQUAL(status, NO_MINING_FEE);
    ledger.closeDB();
    ledger.deleteDB();     
}

TEST(checks_duplicate_nonce) {
    Block b;
    Ledger ledger;
    ledger.init("./data/tmpdb");
    LedgerState deltas;
    ExecutionStatus status;
    User miner;
    User receiver;
    Transaction t1 = miner.mine(1);
    Transaction t2 = miner.send(receiver, BMB(30.0), 1);
    t2.setNonce(t1.getNonce());
    b.addTransaction(t1);
    b.addTransaction(t2);

    status = Executor::ExecuteBlock(b, ledger, deltas);
    ASSERT_EQUAL(status, INVALID_TRANSACTION_NONCE);   
    ledger.closeDB();
    ledger.deleteDB();  
}

TEST(check_valid_send) {
    Block b;

    Ledger ledger;
    ledger.init("./data/tmpdb");
    LedgerState deltas;
    ExecutionStatus status;

    User miner;
    User receiver;

    Transaction t = miner.mine(1);
    b.addTransaction(t);
    Transaction t2 = miner.send(receiver, BMB(30), 1);
    b.addTransaction(t2);

    status = Executor::ExecuteBlock(b, ledger, deltas);
    ASSERT_EQUAL(status, SUCCESS);

    PublicWalletAddress aKey = miner.getAddress(); 
    PublicWalletAddress bKey = receiver.getAddress();

    ASSERT_EQUAL(ledger.getWalletValue(aKey), BMB(20.0))
    ASSERT_EQUAL(ledger.getWalletValue(bKey), BMB(30.0))
    ledger.closeDB();
    ledger.deleteDB();
}

TEST(check_low_balance) {
    Block b;

    Ledger ledger;
    ledger.init("./data/tmpdb");
    LedgerState deltas;
    ExecutionStatus status;
    
    User miner;
    User receiver;

    Transaction t = miner.mine(1);
    b.addTransaction(t);

    Transaction t2 = miner.send(receiver, BMB(100.0), 1);
    b.addTransaction(t2);

    status = Executor::ExecuteBlock(b, ledger, deltas);
    ASSERT_EQUAL(status, BALANCE_TOO_LOW);
    ledger.closeDB();
    ledger.deleteDB();     
}


TEST(check_miner_fee) {
    Block b;

    Ledger ledger;
    ledger.init("./data/tmpdb");
    LedgerState deltas;
    ExecutionStatus status;
    
    User miner;
    User receiver;
    User other;

    // add mining transaction twice
    Transaction t = miner.mine(1);
    Transaction t2 = miner.send(receiver, BMB(20), 1);
    miner.signTransaction(t2);
    b.addTransaction(t);
    b.addTransaction(t2);
    status = Executor::ExecuteBlock(b, ledger, deltas);
    ASSERT_EQUAL(status, SUCCESS);
    
    Block b2;
    b2.setId(2);
    Transaction t3 = miner.mine(2);
    Transaction t4 = receiver.send(other, BMB(1), 2);
    t4.setTransactionFee(BMB(10));
    receiver.signTransaction(t4);
    b2.addTransaction(t3);
    b2.addTransaction(t4);
    status = Executor::ExecuteBlock(b2, ledger, deltas);
    
    ASSERT_EQUAL(status, SUCCESS);
    ASSERT_EQUAL(ledger.getWalletValue(other.getAddress()), BMB(1)); 
    ASSERT_EQUAL(ledger.getWalletValue(receiver.getAddress()), BMB(9)); 
    ASSERT_EQUAL(ledger.getWalletValue(miner.getAddress()), BMB(90)); 
    ledger.closeDB();
    ledger.deleteDB();
}



TEST(check_bad_signature) {
    Block b;
    Ledger ledger;
    ledger.init("./data/tmpdb");
    LedgerState deltas;
    ExecutionStatus status;
    
    User miner;
    User receiver;
    Transaction t = miner.mine(b.getId());
    b.addTransaction(t);
    Transaction t2 = miner.send(receiver, BMB(20.0), b.getId());

    // sign with random sig
    User foo;
    t2.sign(foo.getPublicKey(), foo.getPrivateKey());
    b.addTransaction(t2);

    status = Executor::ExecuteBlock(b, ledger, deltas);
    ASSERT_EQUAL(status, INVALID_SIGNATURE);
    ledger.closeDB();
    ledger.deleteDB();
}
