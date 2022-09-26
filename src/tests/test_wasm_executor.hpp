#include "../server/wasm_executor.hpp"
#include "../server/state_store.hpp"
#include "../core/block.hpp"
#include "../core/helpers.hpp"
#include "../core/crypto.hpp"
#include <iostream>
#include <vector>
using namespace std;

TEST(test_simple_program) {
    vector<uint8_t> byteCode = readBytes("src/wasm/simple_nft.wasm");
    WasmExecutor wasm(byteCode);
    StateStore store;
    store.init("./test-data/tmpdb");
    Block curr;
    curr.setId(1);
    wasm.executeBlockWasm(curr, store);
    ASSERT_EQUAL(store.getWallet("owner"), NULL_ADDRESS);

    json args;
    json result = wasm.getInfo(args, store);
    cout<<result.dump()<<endl;
}