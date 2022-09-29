#include <emscripten/emscripten.h>
#include <stdint.h>
#include <stdio.h>
#include "../core/crypto.hpp"
using namespace std;


#ifdef __cplusplus
extern "C" {
#endif
    #include "external_calls.hpp"

    void executeBlock(const char* args) {
        print("Starting wasm test");
        ExecutionStatus err = WASM_ERROR;
        setUint32("key1", 100);
        if (getUint32("key1") != 100) {
            print("setUint32 failed");
            setReturnValue((char*)&err, sizeof(ExecutionStatus));
            return;
        }
        print("set/getUint32 passed");
        setUint64("key2", 101);
        
        if (getUint64("key2") != 101) {
            print("setUint64 failed");
            setReturnValue((char*)&err, sizeof(ExecutionStatus));
            return;
        }
        
        print("set/getUint64 passed");

        uint32_t c = itemCount("key2");

        if (c != 1) {
            print("itemCount failed");
            setReturnValue((char*)&err, sizeof(ExecutionStatus));
            return;
        }

        setUint64("key2", 101, 1);

        c = itemCount("key2");

        if (c != 2) {
            print("add multiple failed");
            setReturnValue((char*)&err, sizeof(ExecutionStatus));
            return;
        }

        print("add multiple passed");

        pop("key2");
        c = itemCount("key2");
        if (c != 1) {
            print("pop failed");
            setReturnValue((char*)&err, sizeof(ExecutionStatus));
            return;
        }
        print("pop passed");

        setUint64("key2", 102, 1);
        removeItem("key2", 0);
        uint64_t curr = getUint64("key2");
        if (curr != 102) {
            print("removeItem failed");
            setReturnValue((char*)&err, sizeof(ExecutionStatus));
            return;
        }

        print("remove passed");

        print("testing wallets");
        setWallet("wallet", NULL_ADDRESS);
        if (getWallet("wallet") != NULL_ADDRESS) {
            print("wallet failed");
            setReturnValue((char*)&err, sizeof(ExecutionStatus));
            return;
        }

        print("wallets passed");

        print("testing sha256");
        setSha256("sha256", NULL_SHA256_HASH);
        if (getSha256("sha256") != NULL_SHA256_HASH) {
            print("sha256 failed");
            setReturnValue((char*)&err, sizeof(ExecutionStatus));
            return;
        }
        print("sha256 passed");
        print("testing bigint");
        Bigint bigint = 100000000;
        setBigint("bigint", bigint);
        if (getBigint("bigint") != bigint) {
            print("bigint failed");
            setReturnValue((char*)&err, sizeof(ExecutionStatus));
            return;
        }

        print("testing bytes");
        vector<uint8_t> bytes;
        
        for(int i = 0; i < 100; i++) {
            bytes.push_back(1);
        }
        
        setBytes("bytes", bytes);
        if (memcmp(getBytes("bytes").data(), bytes.data(), bytes.size()) != 0) {
            print("bytes failed");
            setReturnValue((char*)&err, sizeof(ExecutionStatus));
            return;
        }

        print("finished");
        err = SUCCESS;
        setReturnValue((char*)&err, sizeof(ExecutionStatus));
        print("exiting");
    }

#ifdef __cplusplus
}
#endif
