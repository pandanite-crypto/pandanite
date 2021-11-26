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

TEST(block_hash_consistency) {
    string A = "{\"difficulty\":25,\"id\":4780,\"lastBlockHash\":\"408B78E303A2130FCB5903887467286CE7D320D1080CBE8D3DE8198053B43AD6\",\"merkleRoot\":\"6B6431D62D1095CBB4D20F10168C1B39754D34997C528C3CE92AE68BB0BDE2CB\",\"nonce\":\"4566676B61BAB77BF15A658D6974592194EA453E7EBA975E74B2AD81056B094A\",\"timestamp\":\"1637858747\",\"transactions\":[{\"amount\":500000,\"fee\":0,\"from\":\"\",\"id\":4780,\"nonce\":\"92zgZDU7\",\"timestamp\":\"1637858747\",\"to\":\"00B28B48A25AFFBF80122963F2CB7957571AE2612B0126BF0D\"}]}";
    string B = "{\"difficulty\":25,\"id\":4780,\"lastBlockHash\":\"408B78E303A2130FCB5903887467286CE7D320D1080CBE8D3DE8198053B43AD6\",\"merkleRoot\":\"EB334D4DA3054E7463210F90EC2D3A0B33105F69AC099BADB863DEC97C7A347F\",\"nonce\":\"236278BC9D738823F361906FBD398DBEB7E8EB1C1E18FAD4099477BD3FBDBD62\",\"timestamp\":\"1637858713\",\"transactions\":[{\"amount\":500000,\"fee\":0,\"from\":\"\",\"id\":4780,\"nonce\":\"I2iyrN6f\",\"timestamp\":\"1637858713\",\"to\":\"00D1B6AF7C92153D2B3BD643B53FF4311746FCF889BE8F9078\"}]}";

    Block a(json::parse(A));
    Block b(json::parse(B));
    cout<<SHA256toString(a.getHash())<<endl;
    cout<<SHA256toString(b.getHash())<<endl;
    ASSERT_TRUE(a.getHash() != b.getHash());

}

