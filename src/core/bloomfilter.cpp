#include "bloomfilter.hpp"
#include <array>
#include <iostream>
using namespace std;

struct BloomFilterInfo {
    int numBits;
    uint8_t numHashes;
};

std::array<uint64_t, 2> hashItem(const uint8_t *data,
                             std::size_t len) {
  std::array<uint64_t, 2> hashValue;
  MurmurHash3_x64_128(data, len, 0, hashValue.data());

  return hashValue;
}

inline uint64_t nthHash(uint8_t n,
                        uint64_t hashA,
                        uint64_t hashB,
                        uint64_t filterSize) {
    return (hashA + n * hashB) % filterSize;
}

BloomFilter::BloomFilter(int numBits, uint8_t numHashes) {
    for(int i =0; i < numBits; i++) {
        bits.push_back(false);
    }
    this->numHashes = numHashes;
}


BloomFilter::BloomFilter(char* data) {
    BloomFilterInfo* header = (BloomFilterInfo*)data;
    char* bits = data + sizeof(BloomFilterInfo);
    this->numHashes = header->numHashes;
    for(int i = 0; i < header->numBits/8; i++) {
        char currSet = bits[i];
        for(int j = 0; j < 8; j++) {
            bool state = currSet & (1<<j);
            this->bits.push_back(state);
        }
    }
}


char* BloomFilter::serialize() {
    BloomFilterInfo info;
    info.numBits = this->bits.size();
    info.numHashes = this->numHashes;
    void* data = malloc(sizeof(BloomFilterInfo) + this->bits.size()/8 );
    memcpy(data,&info, sizeof(BloomFilterInfo));
    char *ptr = (char*)data + sizeof(BloomFilterInfo);
    memset(ptr, 0, this->bits.size()/8);
    for(int i = 0; i < this->bits.size()/8; i++) {
        for(int j = 0; j < 8; j++) {
            bool curr = this->bits[i*8+j];
            if (curr) ptr[i] = ptr[i]|1<<j;
        }
    }
    return (char*)data;
}

void BloomFilter::insert(string item) {
    std::array<uint64_t, 2> hashValues = hashItem((const uint8_t*)item.c_str(), item.length());
    for (int n = 0; n < this->numHashes; n++) {
        this->bits[nthHash(n, hashValues[0], hashValues[1], this->bits.size())] = true;
    }
}

bool BloomFilter::contains(string item) {
    std::array<uint64_t, 2> hashValues = hashItem((const uint8_t*)item.c_str(), item.length());

    for (int n = 0; n < this->numHashes; n++) {
        if (!this->bits[nthHash(n, hashValues[0], hashValues[1], this->bits.size())]) {
            return false;
        }
    }
    return true;
}

bool operator==(const BloomFilter& a, const BloomFilter& b) {
    if (a.numHashes != b.numHashes) return false;
    if (a.bits.size() != b.bits.size()) return false;
    for(int i = 0; i < a.bits.size(); i++) {
        if (a.bits[i] != b.bits[i]) return false;
    }
    return true;
}
 