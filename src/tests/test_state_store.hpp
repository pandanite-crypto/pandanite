#include "../server/state_store.hpp"
#include <iostream>
using namespace std;

TEST(serialize_state) {
    State state;
    state.blockId = 5;
    state.lastBlockId = 30;
    state.itemSize = sizeof(uint8_t);
    state.bytes.push_back(10);
    state.bytes.push_back(10);
    state.bytes.push_back(10);
    state.bytes.push_back(10);

    vector<uint8_t> buf = stateToBuffer(state);
    State state2;
    bufferToState(buf, state2);
    ASSERT_EQUAL(state2.blockId, state.blockId);
    ASSERT_EQUAL(state2.lastBlockId, state.lastBlockId);
    ASSERT_EQUAL(state2.itemSize, state.itemSize);
    ASSERT_EQUAL(state2.bytes.size(), state.bytes.size());
    ASSERT_TRUE(memcmp(state2.bytes.data(), state.bytes.data(), state.bytes.size())== 0);
}

TEST(serialize_null) {
    State state;
    state.blockId = 0;
    state.lastBlockId = 0;
    state.itemSize = sizeof(uint8_t);
    state.bytes.push_back(0);
    state.bytes.push_back(0);
    state.bytes.push_back(0);
    state.bytes.push_back(0);

    vector<uint8_t> buf = stateToBuffer(state);
    State state2;
    bufferToState(buf, state2);
    ASSERT_EQUAL(state2.blockId, state.blockId);
    ASSERT_EQUAL(state2.lastBlockId, state.lastBlockId);
    ASSERT_EQUAL(state2.itemSize, state.itemSize);
    ASSERT_EQUAL(state2.bytes.size(), state.bytes.size());
    ASSERT_TRUE(memcmp(state2.bytes.data(), state.bytes.data(), state.bytes.size())== 0);
}

TEST(save_variables) {
    StateStore store;
    store.init("./test-data/tmpdb");
    store.addBlock();
    store.setUint32("test", 32);
    auto vars = store.getCurrentVariables();
    ASSERT_EQUAL(vars.size(), 1);
    ASSERT_TRUE(vars.find("test") != vars.end());
    store.popBlock();
    vars = store.getCurrentVariables();
    ASSERT_EQUAL(vars.size(), 0);
    store.closeDB();
    store.deleteDB();
}

TEST(test_uint32) {
    StateStore store;
    store.init("./test-data/tmpdb");
    store.addBlock();
    store.setUint32("test", 32);
    uint32_t val = store.getUint32("test");
    ASSERT_EQUAL(val, 32);
    store.addBlock();
    val = store.getUint32("test");
    ASSERT_EQUAL(val, 32);
    store.setUint32("test", 50);
    val = store.getUint32("test");
    ASSERT_EQUAL(val, 50);
    store.popBlock();
    val = store.getUint32("test");
    ASSERT_EQUAL(val, 32);
    bool throws = false;
    store.popBlock();
    try {
        val = store.getUint32("test");
    } catch (...) {
        throws = true;
    }
    ASSERT_TRUE(throws);
    store.closeDB();
    store.deleteDB();
}

TEST(test_int_array) {
    StateStore store;
    store.init("./test-data/tmpdb");
    store.addBlock();
    // store an array of 3 uint32_t's
    store.setUint32("test", 1 , 0);
    store.setUint32("test", 2 , 1);
    store.setUint32("test", 3 , 2);
    ASSERT_EQUAL(3, store.count("test"));

    ASSERT_EQUAL(1, store.getUint32("test", 0));
    ASSERT_EQUAL(2, store.getUint32("test", 1));
    ASSERT_EQUAL(3, store.getUint32("test", 2));

    store.setUint32("test", 101 , 0);
    store.setUint32("test", 102 , 1);
    store.setUint32("test", 103 , 2);
    ASSERT_EQUAL(101, store.getUint32("test", 0));
    ASSERT_EQUAL(102, store.getUint32("test", 1));
    ASSERT_EQUAL(103, store.getUint32("test", 2));

    store.remove("test", 1);
    ASSERT_EQUAL(101, store.getUint32("test", 0));
    ASSERT_EQUAL(103, store.getUint32("test", 1));
    ASSERT_EQUAL(store.count("test"), 2);
    store.closeDB();
    store.deleteDB();
}

TEST(test_inconsistent_array) {
    StateStore store;
    store.init("./test-data/tmpdb");
    store.addBlock();
    // store an array of 3 uint32_t's
    store.setUint32("test", 1 , 0);
    store.setUint32("test", 2 , 1);
    store.setUint32("test", 3 , 2);
    ASSERT_EQUAL(3, store.count("test"));

    bool throws = false;
    try {
        store.setUint64("test", 1, 0);
    } catch (...) {
        throws = true;
    }
    ASSERT_TRUE(throws);
    store.closeDB();
    store.deleteDB();
}

TEST(test_structs) {
    StateStore store;
    store.init("./test-data/tmpdb");
    store.addBlock();
    // store an array of 3 uint32_t's
    vector<uint8_t> testVector;
    for(int i = 0; i < 100; i++) {
        testVector.push_back(i);
    }
    store.setBytes("test", testVector);

    vector<uint8_t> ret = store.getBytes("test");

    ASSERT_EQUAL(ret.size(), testVector.size());
    ASSERT_TRUE(memcmp(ret.data(), testVector.data(), ret.size()) == 0);
    
    store.closeDB();
    store.deleteDB();
}
