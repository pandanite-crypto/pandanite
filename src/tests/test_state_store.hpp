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
    // val = store.getUint32("test");
    // ASSERT_EQUAL(val, 32);
    // bool throws = false;
    // store.popBlock();
    // try {
    //     val = store.getUint32("test");
    // } catch (...) {
    //     throws = true;
    // }
    // ASSERT_TRUE(throws);
    // store.closeDB();
    // store.deleteDB();
}
