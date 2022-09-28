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



void print_str(wasm_exec_env_t exec_env, char* str, int sz) {
    printf("%s\n", str);
}

void setUint32(wasm_exec_env_t exec_env, char* key, uint32_t value, uint32_t idx) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    state->setUint32(key, value, idx);
}
void setUint64(wasm_exec_env_t exec_env, char* key, uint64_t value, uint32_t idx) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    state->setUint64(key, value, idx);
}
void pop(wasm_exec_env_t exec_env, char* key) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    state->pop(string(key));
}
void removeItem(wasm_exec_env_t exec_env, char* key, uint32_t idx) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    state->remove(string(key), idx);
}
uint32_t itemCount(wasm_exec_env_t exec_env, char* key) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    return state->count(string(key));
}

void setSha256(wasm_exec_env_t exec_env, char* key, char* val, uint32_t idx = 0) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    state->setSha256(key, stringToSHA256(string(val)), idx);
}
void setWallet(wasm_exec_env_t exec_env, char* key, char* val, uint32_t idx = 0) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    state->setWallet(key, stringToWalletAddress(string(val)), idx);
}
void setBytes(wasm_exec_env_t exec_env, char* key, char* bytes, uint32_t sz, uint32_t idx = 0) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    // TODO: CLEANUP
    vector<uint8_t> vecbytes;
    for (int i = 0; i < sz; i ++) {
        vecbytes.push_back(bytes[i]);
    }
    state->setBytes(key, vecbytes, idx);
}
void setBigint(wasm_exec_env_t exec_env, char* key, char* val, uint32_t idx = 0) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    Bigint final = Bigint(string(val));
    state->setBigint(key, final, idx);
}

uint64_t getUint64(wasm_exec_env_t exec_env, char* key, uint32_t index = 0) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    return state->getUint64(key, index);
}
uint32_t getUint32(wasm_exec_env_t exec_env, char* key, uint32_t index = 0) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    return state->getUint32(key, index);
}
void getSha256(wasm_exec_env_t exec_env, char* key, char* out, uint32_t sz, int64_t index = 0) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    char* ret = (char*)SHA256toString(state->getSha256(key, index)).c_str();
    strcpy((char*)out, ret);
}
void getBigint(wasm_exec_env_t exec_env, char* key, char* out, uint32_t sz, uint32_t index = 0) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    char* ret = (char*)to_string(state->getBigint(key, index)).c_str();
    strcpy((char*)out, ret);
}
void getWallet(wasm_exec_env_t exec_env, char* key, char* out, uint32_t sz, uint32_t index = 0) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    char* ret = (char*)walletAddressToString(state->getWallet(key, index)).c_str();
    strcpy((char*)out, ret);
}

void getBytes(wasm_exec_env_t exec_env, char* key, char* out, uint32_t sz, uint32_t index = 0) {
    StateStore* state = (StateStore*) wasm_runtime_get_function_attachment(exec_env);
    auto bytes = state->getBytes(key, index);
    memcpy(out, bytes.data(), bytes.size());
}

void setReturnValue(wasm_exec_env_t exec_env, char* out, uint32_t sz) {
    vector<uint8_t>* buf = (vector<uint8_t>*) wasm_runtime_get_function_attachment(exec_env);
    for(int i = 0; i < sz; i++) {
        buf->push_back(out[i]);
    }
}

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
ExecutionStatus WasmExecutor::executeBlockWasm(Block& b, StateStore& state) const {
    char ret[4096];
    static NativeSymbol native_symbols[] = {
        {
            "print_str",
            (void*)print_str,
            "(*~)",
            NULL
        },
        {
            "pop",
            (void*)pop,
            "($)",
            (void*)&state
        },
        {
            "removeItem",
            (void*)removeItem,
            "($i)",
            (void*)&state
        },
        {
            "count",
            (void*)itemCount,
            "($)i",
            (void*)&state
        },
        {
            "_setWallet",
            (void*)setWallet,
            "($$i)",
            (void*)&state
        },
        {
            "_setSha256",
            (void*)setSha256,
            "($$i)",
            (void*)&state
        },
        {
            "_setBytes",
            (void*)setBytes,
            "($$i)",
            (void*)&state
        },
        {
            "_setBigint",
            (void*)setBigint,
            "($$i)",
            (void*)&state
        },
        {
            "setUint32",
            (void*)setUint32,
            "($ii)",
            (void*)&state
        },
        {
            "setUint64",
            (void*)setUint64,
            "($Ii)",
            (void*)&state
        },
        {
            "getUint32",
            (void*)setUint32,
            "($i)i",
            (void*)&state
        },
        {
            "getUint64",
            (void*)setUint64,
            "($i)I",
            (void*)&state
        },
        {
            "_getSha256",
            (void*)getSha256,
            "($*~i)",
            (void*)&state
        },
        {
            "_getBigint",
            (void*)getBigint,
            "($*~i)",
            (void*)&state
        },
        {
            "_getWallet",
            (void*)getBigint,
            "($*~i)",
            (void*)&state
        },
        {
            "_getBytes",
            (void*)getBytes,
            "($*~i)",
            (void*)&state
        },
        {
            "setReturnValue",
            (void*)setReturnValue,
            "(*~)",
            (void*)ret
        }
    };

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
