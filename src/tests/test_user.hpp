#include "../core/user.hpp"
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include "../core/common.hpp"
#include <iostream>
#include <map>
using namespace std;

TEST(check_signature) {
    Block b;
    
    User miner;
    User receiver;

    Transaction t = miner.mine(b.getId());
    b.addTransaction(t);
    Transaction t2 = miner.send(receiver, BMB(30.0), b.getId());
    b.addTransaction(t2);

    ASSERT_TRUE(t2.signatureValid());
}


TEST(check_user_serialization) {
    Block b;
    
    User miner;
    User receiver;

    Transaction t = miner.mine(b.getId());
    b.addTransaction(t);
    Transaction t2 = miner.send(receiver, BMB(30.0), b.getId());
    b.addTransaction(t2);

    ASSERT_TRUE(t2.signatureValid());

    // recreate miner from JSON
    string serialized = miner.toJson().dump();
    json parsed = json::parse(serialized);

    User minerCopy = User(parsed);
    // test the signature still works
    minerCopy.signTransaction(t2);
    ASSERT_TRUE(t2.signatureValid());

}
