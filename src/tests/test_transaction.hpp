#include "../core/user.hpp"
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include "nlohmann/json.hpp"
#include <ctime>
#include <iostream>
using namespace std;


TEST(check_transaction_json_serialization) {

    User miner;
    User receiver;

    Transaction t = miner.mine(1);
    Transaction t2 = miner.send(receiver, 30.0, 1);
    
    ASSERT_TRUE(t2.signatureValid());

    // test the send transaction
    time_t ts = t2.getTimestamp();
    string serialized = t2.toJson().dump();
    json parsed = json::parse(serialized);
    Transaction deserialized = Transaction(parsed);

    ASSERT_TRUE(deserialized.signatureValid());
    ASSERT_TRUE(t2 == deserialized);
    ASSERT_EQUAL(ts, deserialized.getTimestamp());

    // test mining transaction
    serialized = t.toJson().dump();
    
    parsed = json::parse(serialized);
    deserialized = Transaction(parsed);
    ts = t.getTimestamp();
    ASSERT_TRUE(t.toString() == deserialized.toString());
    ASSERT_TRUE(t == deserialized);
    ASSERT_EQUAL(ts, deserialized.getTimestamp());
}

TEST(check_transaction_copy) {

    User miner;
    User receiver;

    Transaction t = miner.mine(1);
    Transaction t2 = miner.send(receiver, 30.0, 1);
    
    Transaction a = t;
    Transaction b = t2;
    ASSERT_TRUE(a==t);
    ASSERT_TRUE(b==t2);

}
