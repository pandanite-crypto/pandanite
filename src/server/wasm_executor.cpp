#include "wasm_executor.hpp"

#include <map>
#include <optional>
#include <iostream>
#include <set>
#include <cmath>
#include "../core/block.hpp"
#include "../core/logger.hpp"
#include "../core/helpers.hpp"
#include "../core/types.hpp"
#include "executor.hpp"
#include "/Users/saliksyed/src/wasm-micro-runtime/core/iwasm/include/wasm_c_api.h"
#include "/Users/saliksyed/src/wasm-micro-runtime/core/iwasm/include/wasm_export.h"
using namespace std;


struct host_methods {
    void setUint32(const char* key, uint32_t value) {
        state->setUint32(key, value);
    }
    void setUint64(const char* key, uint32_t value) {
        state->setUint64(key, value);
    }
    void pop(const char* key) {
        state->pop(key);
    }
    void removeItem(const char* key, const uint64_t idx) {
        state->remove(key, idx);
    }
    uint64_t count(const char* key) {
        return state->count(key);
    }
    void _setSha256(const char* key, const char* val, const uint64_t idx = 0) {
        state->setSha256(key, stringToSHA256(string(val)), idx);
    }
    void _setWallet(const char* key, const char* val, const uint64_t idx = 0) {
        state->setWallet(key, stringToWalletAddress(string(val)), idx);
    }
    void _setBytes(const char* key, const char* bytes, const uint64_t idx = 0) {
        // state->setBytes(key, bytes, idx);
    }
    void _setBigint(const char* key, const char* val, const uint64_t idx = 0) {
        // state->setBigint(key, val, idx);
    }
    uint64_t getUint64(const char* key, uint64_t index = 0) {
        return state->getUint64(key, index);
    }
    uint32_t getUint32(const char* key, uint64_t index = 0) {
        return state->getUint32(key, index);
    }
    void _getSha256(const char* key, int64_t index = 0) {
        const char* ret = SHA256toString(state->getSha256(key, index)).c_str();
        strcpy((char*)returnValue, ret);
    }
    void _getBigint(const char* key, uint64_t index = 0) {
        const char* ret = to_string(state->getBigint(key, index)).c_str();
        strcpy((char*)returnValue, ret);
    }
    void _getWallet(const char* key, uint64_t index = 0) {
        const char* ret = walletAddressToString(state->getWallet(key, index)).c_str();
        strcpy((char*)returnValue, ret);
    }
    
    void _getBytes(const char* key, uint64_t index = 0) const {
        auto bytes = state->getBytes(key, index);
        const char* ret = hexEncode((char*)bytes.data(), bytes.size()).c_str();
        strcpy((char*)returnValue, ret);
    }

    void setReturnValue(const char* val) {
        strcpy(returnValue, val);
    }

    StateStore* state;
    char returnValue[4096];
};

WasmExecutor::WasmExecutor(vector<uint8_t> byteCode) {
    this->byteCode = byteCode;
}

Block WasmExecutor::getGenesis() const {
    return Block();
}

json WasmExecutor::getInfo(json args, StateStore& store) const {
}

TransactionAmount WasmExecutor::getMiningFee(uint64_t blockId, StateStore& store) const {
    throw std::runtime_error("Not used for WASM executor");
}

int WasmExecutor::updateDifficulty(int initialDifficulty, uint64_t numBlocks, const Program& program, StateStore& store) const{
    throw std::runtime_error("Not used for WASM executor");
}

void WasmExecutor::rollback(Ledger& ledger, LedgerState& deltas, StateStore& store) const{
    /** NOTE: WasmExecutor does not use ledger, txdb, or blockStore and stores all data in store **/
    store.popBlock();
}

ExecutionStatus WasmExecutor::executeTransaction(Ledger& ledger, const Transaction t, LedgerState& deltas, StateStore& store) const {
    store.addBlock();
    Block b;
    b.addTransaction(t);
    return this->executeBlockWasm(b, store);
}

void WasmExecutor::rollbackBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, BlockStore& blockStore, StateStore& store) const{
    /** NOTE: WasmExecutor does not use ledger, txdb, or blockStore and stores all data in store **/
    store.popBlock();
}

ExecutionStatus WasmExecutor::executeBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas, StateStore& store) const{
    /** NOTE: WasmExecutor does not use ledger, txdb, or blockStore and stores all data in store **/
    return this->executeBlockWasm(curr, store);
}
#define own
own wasm_trap_t* hello_callback(
  const wasm_val_vec_t* args, wasm_val_vec_t* results
) {
  printf("HELLO HELLO HELLO");
  printf("> Hello World!\n");
  return NULL;
}

void print_str(wasm_exec_env_t exec_env, char* str) {
    printf("FOOBAR1\n");
}

static NativeSymbol native_symbols[] = {
    {
        "print_str", // the name of WASM function name
        (void*)print_str,   // the native function pointer
        "(i)",  // the function prototype signature, avoid to use i32
        NULL        // attachment is NULL
    },
};

ExecutionStatus WasmExecutor::executeBlockWasm(Block& b, StateStore& state) const {
    char error[4096];
    static char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
    init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
    init_args.native_module_name = "env";
    init_args.native_symbols = native_symbols;
    
    wasm_runtime_full_init(&init_args);

    wasm_module_t module = wasm_runtime_load((uint8_t*)this->byteCode.data(), this->byteCode.size(),error, 4096);
    wasm_module_inst_t runtime = wasm_runtime_instantiate(module, 1000000, 1000000,error, 4096);
    wasm_function_inst_t func = wasm_runtime_lookup_function(runtime, "executeBlock", "none");
    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(runtime, 100000);

    char native_buffer[100];
    const char* str  = "hello world";
    strcpy(native_buffer, str);
    uint32_t wasm_buffer = wasm_runtime_module_malloc(runtime, 100, (void **)&native_buffer);
    uint32_t args[1];
    args[0] = wasm_buffer;
    if (wasm_runtime_call_wasm(exec_env, func, 1, args)) {
        std::cout<<"SUCCESS"<<endl;
    }
    
    //wasm_byte_vec_delete(&binary);

    // // Instantiate.
    // printf("Instantiating module...\n");
    // wasm_extern_t* externs[] = {  };
    // wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    // own wasm_instance_t* instance = wasm_instance_new(store, module, &imports, NULL);
    // if (!instance) {
    //     printf("> Error instantiating module!\n");
    //     return WASM_ERROR;
    // }

    // // Extract export.
    // printf("Extracting export...\n");
    // own wasm_extern_vec_t exports;
    // wasm_instance_exports(instance, &exports);
    // if (exports.size == 0) {
    //     printf("> Error accessing exports!\n");
    //     return WASM_ERROR;
    // }

    // const wasm_func_t* add_func = wasm_extern_as_func(exports.data[0]);
    // if (add_func == NULL) {
    //     printf("> Error accessing export!\n");
    //     return WASM_ERROR;
    // }

    // wasm_module_delete(module);
    // wasm_instance_delete(instance);

    // // Call.
    // printf("Calling export...\n");
    // wasm_val_t args[2] = { WASM_I32_VAL(3), WASM_I32_VAL(4) };
    // wasm_val_vec_t args_vec = WASM_ARRAY_VEC(args);

    // wasm_val_t results[1] = { WASM_INIT_VAL };
    // wasm_val_vec_t results_vec = WASM_ARRAY_VEC(results);

    // if (wasm_func_call(add_func, &args_vec, &results_vec)
    //     || results_vec.data[0].of.i32 != 7) {
    //     printf("> Error calling function!\n");
    //     return WASM_ERROR;
    // }

    // wasm_extern_vec_delete(&exports);

    // // Shut down.
    // printf("Shutting down...\n");
    // wasm_store_delete(store);
    // wasm_engine_delete(engine);

    // // All done.
    // printf("Done.\n");

    return SUCCESS;
}
