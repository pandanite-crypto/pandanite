#include "../server/wasm_executor.hpp"
#include "../server/state_store.hpp"
#include "../core/block.hpp"
#include "../core/helpers.hpp"
#include "../core/crypto.hpp"
#include <iostream>
#include <vector>
using namespace std;

TEST(test_simple_program) {
    vector<uint8_t> byteCode = readBytes("src/wasm/simple_example.wasm");
    WasmExecutor wasm(byteCode);
    StateStore store;
    store.init("./test-data/tmpdb");
    Block curr;
    wasm.executeBlockWasm(curr, store);
    ASSERT_EQUAL(store.getUint32("test"), 100);
}