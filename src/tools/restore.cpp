#include <App.h>
#include <string>
#include <mutex>
#include <thread>
#include <string_view>
#include <atomic>
#include <experimental/filesystem>
#include "../core/logger.hpp"
#include "../core/crypto.hpp"
#include "../core/host_manager.hpp"
#include "../core/helpers.hpp"
#include "../core/request_manager.hpp"
#include "../core/api.hpp"
#include "../core/crypto.hpp"
using namespace std;

int main(int argc, char **argv) {    
    std::ifstream input( "backup.txt" );
    size_t numBlocks = 0;
    BlockStore blockStore;
    Ledger ledger;
    ledger.init(LEDGER_FILE_PATH);
    blockStore.init(BLOCK_STORE_FILE_PATH);
    for(std::string line; getline( input, line ); ) {
        numBlocks++;
        Block b (json::parse(line));
        blockStore.setBlock(b);
        blockStore.setBlockCount(numBlocks);
        LedgerState deltasFromBlock;
        ExecutionStatus status = Executor::ExecuteBlock(b, ledger, deltasFromBlock);
        if (status != SUCCESS) {
            Logger::logStatus("Failed to execute block");
        }
    }
}

