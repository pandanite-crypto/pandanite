#include "../server/wasm_executor.hpp"
#include "../server/state_store.hpp"
#include "../core/block.hpp"
#include "../core/helpers.hpp"
#include "../core/crypto.hpp"
#include "../shared/user.hpp"
#include <iostream>
#include <vector>
using namespace std;

const char* KEY = R""""( {
    "privateKey": "08C8D4DAE9A4AE9C456BBB4289883D766DF96DE7E7F44E8F6AC4864A3B9E58539C8307267EF73FD0170145982AE6C0DF14BBFB3355CF3FCEBCE998ECE3BB820D",
    "publicKey": "8F53331361C885D1E9ED3BA0AE465226AE368BA8DB3DCC4875B2D98FB0AE5B01",
    "wallet": "00CF74F1F0111F43E305283219C9DF908D70A14A528083B77A"
})"""";


TEST(test_simple_program) {
    vector<uint8_t> byteCode = readBytes("src/wasm/test_basic.wasm");
    WasmExecutor wasm(byteCode);
    StateStore store;
    store.init("./test-data/tmpdb");
    store.addBlock();
    Block curr;
    curr.setId(1);
    ExecutionStatus status = wasm.executeBlockWasm(curr, store);
    ASSERT_EQUAL(status, SUCCESS);
    store.closeDB();
    store.deleteDB();
}

TEST(test_simple_nft) {
    vector<uint8_t> byteCode = readBytes("src/wasm/simple_nft.wasm");
    WasmExecutor wasm(byteCode);
    StateStore store;
    store.init("./test-data/tmpdb");
    store.addBlock();
    Block curr;
    curr.setId(1);
    ExecutionStatus status = wasm.executeBlockWasm(curr, store);
    store.addBlock();
    ASSERT_EQUAL(status, SUCCESS);
    json args;
    json info = wasm.getInfo(args, store);
    ASSERT_EQUAL(info["owner"], "00CF74F1F0111F43E305283219C9DF908D70A14A528083B77A");
    json keys = json::parse(string(KEY));
    User recepient;
    User sender(keys);
    
    // send the NFT to a new user
    Transaction tx = sender.send(recepient, 1);
    Block next;
    next.setId(2);
    next.addTransaction(tx);
    status = wasm.executeBlockWasm(next, store);
    ASSERT_EQUAL(status, SUCCESS);
    info = wasm.getInfo(args, store);
    PublicWalletAddress receiverWallet = walletAddressFromPublicKey(recepient.getPublicKey());
    ASSERT_EQUAL(info["owner"], walletAddressToString(receiverWallet));
    store.closeDB();
    store.deleteDB();
}