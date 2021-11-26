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
    HostManager h;
    BlockChain blockchain(h);
    for(std::string line; getline( input, line ); ) {
        numBlocks++;
        if (numBlocks == 1) continue;
        Block b (json::parse(line));
        ExecutionStatus status = blockchain.addBlock(b);
        if (status != SUCCESS) {
            Logger::logStatus("Failed to execute block : " + executionStatusAsString(status));
            Logger::logStatus("Made it to block : " + std::to_string(numBlocks));
            break;
        }
    }
}

