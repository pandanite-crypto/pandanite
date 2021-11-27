#include "../external/murmurhash3/MurmurHash3.hpp"
#include <string>
#include <vector>
using namespace std;

class BloomFilter {
    public:
        BloomFilter(int numBits, uint8_t numHashes);
        BloomFilter(char* serialized);
        bool contains(string item);
        void insert(string item);
        char* serialize();
    protected:
        vector<bool> bits;
        uint8_t numHashes;
        friend bool operator==(const BloomFilter& a, const BloomFilter& b);
};

bool operator==(const BloomFilter& a, const BloomFilter& b);