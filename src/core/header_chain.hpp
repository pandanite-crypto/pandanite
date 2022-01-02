#pragma once

class HeaderChain {
    public:
        HeaderChain(host);
        void load();
        bool valid();
        string getHost();
        uint64_t getTotalWork();
        uint64_t getChainLength();
    protected:
        string host;
        uint64_t totalWork;
        uint64_t chainLength;
        bool failed;
};