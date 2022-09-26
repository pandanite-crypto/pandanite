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
#include "../external/eosio/vm/backend.hpp"
#include "../external/eosio/vm/error_codes.hpp"
#include "../external/eosio/vm/host_function.hpp"
#include "../external/eosio/vm/watchdog.hpp"
#include "executor.hpp"
using namespace eosio;
using namespace eosio::vm;
using namespace std;

namespace eosio { namespace vm {
    template <>
    struct wasm_type_converter<const char*> : linear_memory_access {
        const char* from_wasm(const void* val) { validate_c_str(val); return static_cast<const char*>(val); }
        void to_wasm(const char* val) {}
    };

    template<typename T>
    struct wasm_type_converter<T*> {
        static T* from_wasm(void* val) {
            return (T*)val;
        }
        static void* to_wasm(T* val) {
            return (void*)val;
        }
    };

    template<typename T>
    struct wasm_type_converter<T&> {
        static T& from_wasm(T* val) {
            return *val;
        }
        static T* to_wasm(T& val) {
        return std::addressof(val);
        }
    };
}}

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
    wasm_allocator wa;
    using backend_t = eosio::vm::backend<host_methods>;
    using rhf_t     = eosio::vm::registered_host_functions<host_methods>;
    rhf_t::add<host_methods, &host_methods::setUint32, wasm_allocator>("env", "setUint32");
    rhf_t::add<host_methods, &host_methods::setUint64, wasm_allocator>("env", "setUint64");
    rhf_t::add<host_methods, &host_methods::pop, wasm_allocator>("env", "pop");
    rhf_t::add<host_methods, &host_methods::removeItem, wasm_allocator>("env", "removeItem");
    rhf_t::add<host_methods, &host_methods::count, wasm_allocator>("env", "count");
    rhf_t::add<host_methods, &host_methods::_setSha256, wasm_allocator>("env", "setSha256");
    rhf_t::add<host_methods, &host_methods::_setWallet, wasm_allocator>("env", "setWallet");
    rhf_t::add<host_methods, &host_methods::_setBytes, wasm_allocator>("env", "setBytes");
    rhf_t::add<host_methods, &host_methods::_setBigint, wasm_allocator>("env", "setBigint");
    rhf_t::add<host_methods, &host_methods::getUint64, wasm_allocator>("env", "getUint64");
    rhf_t::add<host_methods, &host_methods::setUint32, wasm_allocator>("env", "getUint32");
    rhf_t::add<host_methods, &host_methods::getUint64, wasm_allocator>("env", "getUint64");
    rhf_t::add<host_methods, &host_methods::_getSha256, wasm_allocator>("env", "getSha256");
    rhf_t::add<host_methods, &host_methods::_getBigint, wasm_allocator>("env", "getBigint");
    rhf_t::add<host_methods, &host_methods::_getWallet, wasm_allocator>("env", "getWallet");
    rhf_t::add<host_methods, &host_methods::_getBytes, wasm_allocator>("env", "getBytes");
    watchdog wd{std::chrono::seconds(3)};
    try {
        vector<uint8_t> bytes = this->byteCode;
        backend_t bkend(bytes, rhf_t{});
        bkend.set_wasm_allocator(&wa);
        bkend.initialize();
        
        host_methods ehm;
        ehm.state = &store;
        char jsonBuffer[4096];
        strcpy(jsonBuffer, args.dump().c_str());
        bkend.call_with_return(&ehm, "env", "getInfo", jsonBuffer);
        return json::parse(string(ehm.returnValue));
    } catch (std::exception& e) { 
        std::cerr << e.what(); 
        json ret;
        ret["error"] = "WASM execution failed";
        return ret;
    }
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

ExecutionStatus WasmExecutor::executeBlockWasm(Block& b, StateStore& store) const {
    wasm_allocator wa;
    using backend_t = eosio::vm::backend<host_methods>;
    using rhf_t     = eosio::vm::registered_host_functions<host_methods>;
    rhf_t::add<host_methods, &host_methods::setUint32, wasm_allocator>("env", "setUint32");
    rhf_t::add<host_methods, &host_methods::setUint64, wasm_allocator>("env", "setUint64");
    rhf_t::add<host_methods, &host_methods::pop, wasm_allocator>("env", "pop");
    rhf_t::add<host_methods, &host_methods::removeItem, wasm_allocator>("env", "removeItem");
    rhf_t::add<host_methods, &host_methods::count, wasm_allocator>("env", "count");
    rhf_t::add<host_methods, &host_methods::setReturnValue, wasm_allocator>("env", "setReturnValue");
    rhf_t::add<host_methods, &host_methods::_setSha256, wasm_allocator>("env", "setSha256");
    rhf_t::add<host_methods, &host_methods::_setWallet, wasm_allocator>("env", "setWallet");
    rhf_t::add<host_methods, &host_methods::_setBytes, wasm_allocator>("env", "setBytes");
    rhf_t::add<host_methods, &host_methods::_setBigint, wasm_allocator>("env", "setBigint");
    rhf_t::add<host_methods, &host_methods::getUint64, wasm_allocator>("env", "getUint64");
    rhf_t::add<host_methods, &host_methods::setUint32, wasm_allocator>("env", "getUint32");
    rhf_t::add<host_methods, &host_methods::getUint64, wasm_allocator>("env", "getUint64");
    rhf_t::add<host_methods, &host_methods::_getSha256, wasm_allocator>("env", "getSha256");
    rhf_t::add<host_methods, &host_methods::_getBigint, wasm_allocator>("env", "getBigint");
    rhf_t::add<host_methods, &host_methods::_getWallet, wasm_allocator>("env", "getWallet");
    rhf_t::add<host_methods, &host_methods::_getBytes, wasm_allocator>("env", "getBytes");
    watchdog wd{std::chrono::seconds(3)};
    try {
        vector<uint8_t> bytes = this->byteCode;
        backend_t bkend(bytes, rhf_t{});
        bkend.set_wasm_allocator(&wa);
        bkend.initialize();
        host_methods ehm;
        ehm.state = &store;
        auto ret = bkend.call_with_return(&ehm, "env", "executeBlock");
        return (ExecutionStatus) ret->to_ui32();
    } catch (std::exception& e) { 
        std::cerr << e.what(); 
        return WASM_ERROR;
    }

   return SUCCESS;
}
