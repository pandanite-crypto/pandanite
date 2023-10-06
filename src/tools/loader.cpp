#include "../core/common.hpp"
#include "../core/host_manager.hpp"
#include "../server/blockchain.hpp"
#include "spdlog/spdlog.h"

#include <fstream>
using namespace std;

int main(int argc, char** argv) {
    spdlog::info("=====LOAD BLOCKCHAIN====");
    string line;
    ifstream infile(argv[1]);
    spdlog::info("Loading from {}", argv[1]);
    if (infile.is_open()) {
        HostManager h;
        BlockChain* blockchain = new BlockChain(h);
        while (std::getline(infile, line)) {    
            try {
                json data = json::parse(line);
                Block b = Block(data);
                blockchain->addBlock(b);
                spdlog::info("Added block: {}", data.dump());
            } catch (...){
                spdlog::error("Error parsing data file. Please delete data dir and try again");
                break;
            }
        }
    } else {
        spdlog::error("Failed to load file.");
    }
    
}
