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
using namespace std;

#define RETURN_KEY "__RETURN__"

#define STACK_SZ 1000000

void print_str(wasm_exec_env_t exec_env, char* str, uint32_t sz) {
    printf("%s\n", str);
}

void setUint32(wasm_exec_env_t exec_env, char* key, uint32_t value, uint32_t idx) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    state->setUint32(key, value, idx);
}
void setUint64(wasm_exec_env_t exec_env, char* key, uint64_t value, uint32_t idx) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    string fkey = string(key);
    state->setUint64(fkey, value, idx);
}
void pop(wasm_exec_env_t exec_env, char* key) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    state->pop(key);
}
void removeItem(wasm_exec_env_t exec_env, char* key, uint32_t idx) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    state->remove(string(key), idx);
}
uint32_t itemCount(wasm_exec_env_t exec_env, char* key) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    string fkey = string(key);
    return state->count(fkey);
}

void setSha256(wasm_exec_env_t exec_env, char* key, char* val, uint32_t idx = 0) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    state->setSha256(key, stringToSHA256(string(val, 64)), idx);
}
void setWallet(wasm_exec_env_t exec_env, char* key, char* val, uint32_t idx = 0) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    state->setWallet(key, stringToWalletAddress(string(val, 50)), idx);
}
void setBytes(wasm_exec_env_t exec_env, char* key, char* bytes, uint32_t sz, uint32_t idx = 0) {
    if (sz >= 4096) throw std::runtime_error("Max 4kb byte size limit");
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    // TODO: CLEANUP
    vector<uint8_t> vecbytes;
    for (int i = 0; i < sz; i ++) {
        vecbytes.push_back(bytes[i]);
    }
    state->setBytes(key, vecbytes, idx);
}

void setBigint(wasm_exec_env_t exec_env, char* key, char* val, uint32_t sz, uint32_t idx = 0) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    Bigint final = Bigint::fromBuffer(val, sz);
    state->setBigint(key, final, idx);
}

uint32_t getBigint(wasm_exec_env_t exec_env, char* key, char* out, uint32_t sz, uint32_t index = 0) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    string fkey = string(key);
    Bigint b = state->getBigint(fkey, index);
    vector<uint8_t> buf = b.toBuffer();
    memcpy(out, buf.data(), buf.size());
    return buf.size();
}

uint64_t getUint64(wasm_exec_env_t exec_env, char* key, uint32_t index = 0) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    return state->getUint64(key, index);
}
uint32_t getUint32(wasm_exec_env_t exec_env, char* key, uint32_t index = 0) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    return state->getUint32(key, index);
}

void getSha256(wasm_exec_env_t exec_env, char* key, char* out, uint32_t sz, int64_t index = 0) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    string s = SHA256toString(state->getSha256(key, index));
    char* ret = (char*)s.c_str();
    strcpy((char*)out, ret);
}

void getWallet(wasm_exec_env_t exec_env, char* key, char* out, uint32_t sz, uint32_t index = 0) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    string s = walletAddressToString(state->getWallet(key, index));
    char* ret = (char*)s.c_str();
    strcpy((char*)out, ret);
}

int getBytes(wasm_exec_env_t exec_env, char* key, char* out, uint32_t sz, uint32_t index = 0) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    auto bytes = state->getBytes(key, index);
    memcpy(out, bytes.data(), bytes.size());
    return (int)bytes.size();
}

void setReturnValue(wasm_exec_env_t exec_env, char* out, uint32_t sz) {
    StateStore* state = ((WasmEnvironment*) wasm_runtime_get_function_attachment(exec_env))->state;
    vector<uint8_t> bytes;
    for(int i = 0; i < sz; i++) {
        bytes.push_back(out[i]);
    }
    state->setBytes(RETURN_KEY, bytes);
}

WasmExecutor::WasmExecutor(vector<uint8_t> byteCode) {
    this->byteCode = byteCode;
}

Block WasmExecutor::getGenesis() const {
    return Block();
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

WasmEnvironment* WasmExecutor::initWasm(StateStore& state) const {
    WasmEnvironment* env = new WasmEnvironment();
    env->wasm_buffer = 0;
    env->state = &state;
    static NativeSymbol native_symbols[] = {
        {
            "print_str",
            (void*)print_str,
            "(*~)",
            NULL
        },
        {
            "setUint32",
            (void*)setUint32,
            "($ii)",
            (void*)env
        },
        {
            "setUint64",
            (void*)setUint64,
            "($Ii)",
            (void*)env
        },
        {
            "getUint32",
            (void*)getUint32,
            "($i)i",
            (void*)env
        },
        {
            "setReturnValue",
            (void*)setReturnValue,
            "(*~)",
            (void*)env
        },
        {
            "getUint64",
            (void*)getUint64,
            "($i)I",
            (void*)env
        },
        {
            "pop",
            (void*)pop,
            "($)",
            (void*)env
        },
        {
            "removeItem",
            (void*)removeItem,
            "($i)",
            (void*)env
        },
        {
            "itemCount",
            (void*)itemCount,
            "($)i",
            (void*)env
        },
        {
            "_setWallet",
            (void*)setWallet,
            "($$i)",
            (void*)env
        },
        {
            "_setSha256",
            (void*)setSha256,
            "($$i)",
            (void*)env
        },
        {
            "_setBytes",
            (void*)setBytes,
            "($$ii)",
            (void*)env
        },
        {
            "_getSha256",
            (void*)getSha256,
            "($*~i)",
            (void*)env
        },
        {
            "_getWallet",
            (void*)getWallet,
            "($*~i)",
            (void*)env
        },
        {
            "_getBytes",
            (void*)getBytes,
            "($*~i)i",
            (void*)env
        },
        {
            "_setBigint",
            (void*)setBigint,
            "($$ii)",
            (void*)env
        },
        {
            "_getBigint",
            (void*)getBigint,
            "($*~i)i",
            (void*)env
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

    if(!wasm_runtime_full_init(&init_args)) {
        std::cout<<"Failed to init runtime"<<std::endl;
        throw std::runtime_error("Wasm error");
    }
    vector<uint8_t> wasmCode = this->byteCode; // make copy
    wasm_module_t wasmModule = wasm_runtime_load((uint8_t*)wasmCode.data(), wasmCode.size(), error, 4096);

    if (!wasmModule) {
        std::cout<<"Failed to init module: "<<error<<std::endl;
    }
    wasm_module_inst_t runtime = wasm_runtime_instantiate(wasmModule, STACK_SZ, STACK_SZ, error, 4096);
    if (!runtime) {
        std::cout<<"Failed to init runtime"<<std::endl;
        throw std::runtime_error("Wasm error");
    }
    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(runtime, 100000);
    if (!exec_env) {
        std::cout<<"Failed to init execution environment"<<std::endl;
        throw std::runtime_error("Wasm error");
    }
    env->module = wasmModule;
    env->runtime = runtime;
    env->executor = exec_env;
    return env;
}

ExecutionStatus WasmExecutor::executeBlock(Block& curr, Ledger& ledger, TransactionStore & txdb, LedgerState& deltas, StateStore& store) const{
    /** NOTE: WasmExecutor does not use ledger, txdb, or blockStore and stores all data in store **/
    return this->executeBlockWasm(curr, store);
}

json WasmExecutor::getInfo(json args, StateStore& store) const{
    json result;
    WasmEnvironment* env = this->initWasm(store);
    wasm_function_inst_t func = wasm_runtime_lookup_function(env->runtime, "getInfo", "none");
    if (wasm_runtime_call_wasm(env->executor, func, 0, NULL)) {  
        vector<uint8_t> ret = env->state->getBytes(RETURN_KEY);
        ExecutionStatus status = *(ExecutionStatus*)(ret.data());
        if (ret.size() == 0) {
            result["error"] = "No data returned";
            this->cleanupWasm(env);
            return result;
        }
        json result = json::parse(string((char*)ret.data(), ret.size()));
        this->cleanupWasm(env);
        return result;
    }
    this->cleanupWasm(env);
    result["error"] = "Could not fetch data";
    return result;
}

void  WasmExecutor::cleanupWasm(WasmEnvironment* env) const {
    env->state->remove(string(RETURN_KEY), 0);
    wasm_runtime_destroy_exec_env(env->executor);
    if (env->wasm_buffer != 0) wasm_runtime_module_free(env->runtime, env->wasm_buffer);
    wasm_runtime_deinstantiate(env->runtime);
    wasm_runtime_unload(env->module);
    wasm_runtime_destroy();
}


ExecutionStatus WasmExecutor::executeBlockWasm(Block& b, StateStore& state) const {

    WasmEnvironment* env = this->initWasm(state);

    vector<uint8_t> ret;
    ExecutionStatus status = WASM_ERROR;
    for(int i = 0; i < sizeof(ExecutionStatus); i++) ret.push_back(0);
    memcpy(ret.data(), &status, sizeof(ExecutionStatus));

    wasm_function_inst_t func = wasm_runtime_lookup_function(env->runtime, "executeBlock", "none");
    uint64_t sz = BLOCKHEADER_BUFFER_SIZE + b.getTransactions().size() * transactionInfoBufferSize(false);
    char* buf = (char*) malloc(sz);
    char* ptr = buf;
    BlockHeader header = b.serialize();
    blockHeaderToBuffer(header, ptr);
    ptr += BLOCKHEADER_BUFFER_SIZE;
    
    for(auto tx : b.getTransactions()) {
        TransactionInfo txinfo = tx.serialize();
        transactionInfoToBuffer(txinfo, ptr, false);
        ptr += transactionInfoBufferSize(false);
    }
    char* native_buf;
    env->wasm_buffer = wasm_runtime_module_malloc(env->runtime, sz, (void **)&native_buf);
    memcpy(native_buf, buf, sz);
    uint32_t args[1];
    args[0] = env->wasm_buffer;
    if (wasm_runtime_call_wasm(env->executor, func, 1, args)) { 
        auto bytes = env->state->getBytes(RETURN_KEY);
        char* buf = (char*)bytes.data();
        ExecutionStatus status = *(ExecutionStatus*)(buf);
        this->cleanupWasm(env);
        return status;
    }
    this->cleanupWasm(env);
    return WASM_ERROR;
}
