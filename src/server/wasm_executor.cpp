#include "wasm_executor.hpp"

#include <map>
#include <optional>
#include <iostream>
#include <set>
#include <cmath>
#include "../core/block.hpp"
#include "../core/logger.hpp"
#include "../core/helpers.hpp"
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
   };

}}

// example of host function as a raw C style function
void eosio_assert(bool test, const char* msg) {
   if (!test) {
      std::cout << msg << std::endl;
      throw 0;
   }
}

void print_num(uint64_t n) { std::cout << "Number : " << n << "\n"; }

struct host_methods {
   // example of a host "method"
   void print_name(const char* nm) { std::cout << "Name : " << nm <<  "\n"; }
   // example of another type of host function
   static void* memset(void* ptr, int x, size_t n) { return ::memset(ptr, x, n); }
   void setUint32(const char* key, uint32_t value) {
        state->setUint32(key, value);
   }
   StateStore* state;
};


WasmExecutor::WasmExecutor(vector<uint8_t> byteCode) {
    this->byteCode = byteCode;
}

Block WasmExecutor::getGenesis() const {
    return Block();
}

TransactionAmount WasmExecutor::getMiningFee(uint64_t blockId, StateStore& store) const {
    return 0;
}

int WasmExecutor::updateDifficulty(int initialDifficulty, uint64_t numBlocks, const Program& program, StateStore& store) const{
    return 0;
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
    return SUCCESS;
}

ExecutionStatus WasmExecutor::executeBlockWasm(Block& b, StateStore& store) const {
   wasm_allocator wa;
   using backend_t = eosio::vm::backend<host_methods>;
   using rhf_t     = eosio::vm::registered_host_functions<host_methods>;
   // register print_num
   rhf_t::add<nullptr_t, &print_num, wasm_allocator>("env", "print_num");
   // register eosio_assert
   rhf_t::add<nullptr_t, &eosio_assert, wasm_allocator>("env", "eosio_assert");
   // register print_name
   rhf_t::add<host_methods, &host_methods::print_name, wasm_allocator>("env", "print_name");
   rhf_t::add<host_methods, &host_methods::setUint32, wasm_allocator>("env", "setUint32");
   // finally register memset
   rhf_t::add<nullptr_t, &host_methods::memset, wasm_allocator>("env", "memset");
   watchdog wd{std::chrono::seconds(3)};
   try {
      vector<uint8_t> bytes = this->byteCode;
      backend_t bkend(bytes, rhf_t{});
      bkend.set_wasm_allocator(&wa);
      bkend.initialize();
      host_methods ehm;
      ehm.state = &store;
      bkend(&ehm, "env", "executeBlock");

   } catch (std::exception& e) { 
        std::cerr << e.what(); 
        return WASM_ERROR;
    }

   return SUCCESS;
}
