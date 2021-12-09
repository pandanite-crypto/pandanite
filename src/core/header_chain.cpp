#include "header_chain.hpp"



HeaderChain::HeaderChain(string host) {
    this->host = host;
    this->totalWork = 0;
    this->syncThread.push_back(std::thread(sync_blocks, ref(*this)));
    this->isAlive = true;
}

HeaderChain::getTotalWork() {
    return this->totalWork;
}

HeaderChain::getHost() {
    return this->totalWork;
}

void sync_blocks(HeaderChain & chain) {
    int failureCount = 0;
    while(true) {
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
    }
}
