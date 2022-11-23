#include "../core/common.hpp"
#include "../core/host_manager.hpp"
#include "../server/blockchain.hpp"

#include <fstream>
using namespace std;

int main(int argc, char** argv) {
    cout<<"=====LOAD BLOCKCHAIN===="<<endl;
    string line;
    ifstream infile(argv[1]);
    cout<<"Loading from "<<argv[1]<<endl;
    if (infile.is_open()) {
        HostManager h;
        BlockChain* blockchain = new BlockChain(h);
        while (std::getline(infile, line)) {    
            try {
                json data = json::parse(line);
                Block b = Block(data);
                blockchain->addBlock(b);
                cout<<"Added block: " << data.dump() << endl;
            } catch (...){
                cout<<"Error parsing data file. Please delete data dir and try again"<<endl;
                break;
            }
        }
    } else {
        cout<<"Failed to load file."<<endl;
    }
    
}
