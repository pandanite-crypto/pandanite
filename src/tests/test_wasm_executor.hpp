#include "../server/wasm_executor.hpp"
#include "../server/state_store.hpp"
#include "../core/block.hpp"
#include "../core/helpers.hpp"
#include "../core/crypto.hpp"
#include <iostream>
#include <vector>
using namespace std;

// TEST(test_simple_program) {
//     vector<uint8_t> byteCode = readBytes("src/wasm/test_basic.wasm");
//     WasmExecutor wasm(byteCode);
//     StateStore store;
//     store.init("./test-data/tmpdb");
//     Block curr;
//     curr.setId(1);
//     ExecutionStatus status = wasm.executeBlockWasm(curr, store);
//     ASSERT_EQUAL(status, SUCCESS);
//     store.closeDB();
//     store.deleteDB();

// }

TEST(test_simple_nft) {
    vector<uint8_t> byteCode = readBytes("src/wasm/simple_nft.wasm");
    WasmExecutor wasm(byteCode);
    StateStore store;
    store.init("./test-data/tmpdb");
    Block curr;
    curr.setId(1);
    ExecutionStatus status = wasm.executeBlockWasm(curr, store);
    ASSERT_EQUAL(store.getWallet("owner"), NULL_ADDRESS);
    store.closeDB();
    store.deleteDB();
}