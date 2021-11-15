#include "../core/user.hpp"
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include "../core/helpers.hpp"
#include "../core/common.hpp"
#include <iostream>

using namespace std;

TEST(check_block_json_serialization) {
    Block a;
    User miner;
    User receiver;
    Transaction t = miner.mine(a.getId());
    a.addTransaction(t);
    // send tiny shares to receiver:
    for(int i = 0; i < 5; i++) {
        a.addTransaction(miner.send(receiver, 1, a.getId()));
    }
    string s = a.toJson().dump();
    json x = json::parse(s);
    Block b(x);
    ASSERT_TRUE(a==b);
}


TEST(check_block_struct_serialization) {
    Block a;
    User miner;
    User receiver;
    Transaction t = miner.mine(a.getId());
    a.addTransaction(t);
    // send tiny shares to receiver:
    for(int i = 0; i < 5; i++) {
        a.addTransaction(miner.send(receiver, 1, a.getId()));
    }
    BlockHeader d = a.serialize();
    Block b(d, a.getTransactions());
    ASSERT_TRUE(a==b);
}