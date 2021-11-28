#pragma once
#include "../external/murmurhash3/MurmurHash3.hpp"
#include <string>
#include <vector>
#include <memory.h>
using namespace std;


struct BloomFilterInfo {
    int numBits;
    uint8_t numHashes;
};


class BloomFilter {
    public:
        BloomFilter();
        BloomFilter(int numBits, uint8_t numHashes);
        BloomFilter(char* serialized);
        bool contains(string item);
        void insert(string item);
        std::pair<char*,size_t> serialize();
        void clear();
    protected:
        vector<bool> bits;
        uint8_t numHashes;
        friend bool operator==(const BloomFilter& a, const BloomFilter& b);
};

bool operator==(const BloomFilter& a, const BloomFilter& b);