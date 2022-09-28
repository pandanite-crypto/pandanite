#include "../shared/user.hpp"
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include "../core/common.hpp"
#include <ctime>
#include <iostream>
using namespace std;


TEST(check_transaction_json_serialization) {

    User miner;
    User receiver;

    Transaction t = miner.mine();
    Transaction t2 = miner.send(receiver, BMB(30.0));
    
    // test layer-2 transaction
    Transaction t3 = t2;

    SHA256Hash testProgramId;

    t3.makeLayer2(testProgramId, std::array<uint8_t, 128>());

    // test program execution transaction
    Transaction t4 = Transaction(receiver.getAddress(), receiver.getPublicKey(), 0, testProgramId);
    
    
    ASSERT_TRUE(t2.signatureValid());

    // test the send transaction
    uint64_t ts = t2.getNonce();
    string serialized = t2.toJson().dump();
    json parsed = json::parse(serialized);
    Transaction deserialized = Transaction(parsed);

    ASSERT_TRUE(deserialized.signatureValid());
    ASSERT_TRUE(t2 == deserialized);
    ASSERT_EQUAL(ts, deserialized.getNonce());

    // test mining transaction
    serialized = t.toJson().dump();
    
    parsed = json::parse(serialized);
    deserialized = Transaction(parsed);
    ts = t.getNonce();
    ASSERT_TRUE(t.hashContents() == deserialized.hashContents());
    ASSERT_TRUE(t == deserialized);
    ASSERT_EQUAL(ts, deserialized.getNonce());

    // test layer 2 transaction
    serialized = t3.toJson().dump();
    parsed = json::parse(serialized);
    deserialized = Transaction(parsed);
    ts = t3.getNonce();
    ASSERT_EQUAL(t3.getNonce(), LAYER_2_TX_FLAG);
    ASSERT_TRUE(t3.hashContents() == deserialized.hashContents());
    ASSERT_TRUE(t3 == deserialized);
    ASSERT_EQUAL(ts, deserialized.getNonce());

    // test program execution transaction
    serialized = t4.toJson().dump();
    parsed = json::parse(serialized);
    deserialized = Transaction(parsed);
    ts = t4.getNonce();
    ASSERT_EQUAL(t4.getNonce(), PROGRAM_CREATE_TX_FLAG);
    ASSERT_TRUE(t4.hashContents() == deserialized.hashContents());
    ASSERT_TRUE(t4 == deserialized);
    ASSERT_EQUAL(ts, deserialized.getNonce());
}

TEST(check_transaction_struct_serialization) {

    User miner;
    User receiver;

    Transaction t = miner.mine();
    Transaction t2 = miner.send(receiver, BMB(30.0));
    
    ASSERT_TRUE(t2.signatureValid());

    // test the send transaction
    uint64_t ts = t2.getNonce();
    TransactionInfo serialized = t2.serialize();
    Transaction deserialized = Transaction(serialized);

    ASSERT_TRUE(deserialized.signatureValid());
    ASSERT_TRUE(t2 == deserialized);
    ASSERT_EQUAL(ts, deserialized.getNonce());

    // test mining transaction
    serialized = t.serialize();
    deserialized = Transaction(serialized);
    ts = t.getNonce();
    ASSERT_TRUE(t.hashContents() == deserialized.hashContents());
    ASSERT_TRUE(t == deserialized);
    ASSERT_EQUAL(ts, deserialized.getNonce());
}

TEST(check_transaction_copy) {

    User miner;
    User receiver;

    Transaction t = miner.mine();
    Transaction t2 = miner.send(receiver, BMB(30.0));
    
    Transaction a = t;
    Transaction b = t2;
    ASSERT_TRUE(a==t);
    ASSERT_TRUE(b==t2);
}


TEST(check_transaction_network_serialization) {
    User miner;
    User receiver;

    Transaction a = miner.mine();
    Transaction b = miner.send(receiver, BMB(30.0));

    TransactionInfo t1 = a.serialize();
    TransactionInfo t2 = b.serialize();
    
    char buf1[TRANSACTIONINFO_BUFFER_SIZE];
    char buf2[TRANSACTIONINFO_BUFFER_SIZE];

    transactionInfoToBuffer(t1, buf1);
    transactionInfoToBuffer(t2, buf2);

    Transaction a2 = Transaction(transactionInfoFromBuffer(buf1));
    Transaction b2 = Transaction(transactionInfoFromBuffer(buf2));

    ASSERT_TRUE(a == a2);
    ASSERT_TRUE(b == b2);
}
