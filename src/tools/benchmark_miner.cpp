#include "../core/common.hpp"
#include "../core/crypto.hpp"
#include <iostream>
#include <ctime>
#include <sys/time.h>
#include <chrono>
#include "../external/sha256/sha2.hpp"
using namespace std;

namespace benchmark {



    void SHA256Fast(const unsigned char* buffer, size_t len, unsigned char* out) {
        sha256_ctx sha256;
        sha256_init(&sha256);
        sha256_update(&sha256, buffer, len);
        sha256_final(&sha256, out);
    }


    inline bool checkLeadingZeroBits(SHA256Hash& hash, unsigned int challengeSize) {
        int bytes = challengeSize / 8;
        const uint8_t * a = hash.data();
        if (bytes == 0) {
            return a[0]>>(8-challengeSize) == 0;
        } else {
            for (int i = 0; i < bytes; i++) {
                if (a[i] != 0) return false;
            }
            int remainingBits = challengeSize - 8*bytes;
            if (remainingBits > 0) return a[bytes + 1]>>(8-remainingBits) == 0;
            else return true;
        }
        return false;
    }


    inline bool verifyHash(SHA256Hash& target, SHA256Hash& nonce, unsigned char challengeSize) {
        SHA256Hash fullHash  = concatHashes(target, nonce);
        return checkLeadingZeroBits(fullHash, challengeSize);
    }

    SHA256Hash mineHash(SHA256Hash targetHash, unsigned char challengeSize, long& numHashes) {
        while(true) {
            SHA256Hash solution;
            for(size_t i = 0; i < solution.size(); i++) {
                solution[i] = rand()%256;
            }
            numHashes++;
            bool found = verifyHash(targetHash, solution, challengeSize);
            if (found) {
                return solution;
            }
        }
    }

    SHA256Hash mineHashShifu(SHA256Hash target, unsigned char challengeSize, long& numHashes) {
    vector<uint8_t> concat;
    concat.resize(2*32);
    for(size_t i = 0; i < 32; i++) concat[i]=target[i];
    // fill with random data for privacy (one cannot guess number of tries later)
    for(size_t i=32; i<64;++i) concat[i]=rand()%256;
        while (true) {
            *reinterpret_cast<uint64_t*>(concat.data()+32)+=1;
            SHA256Hash fullHash;
            SHA256Fast((const unsigned char*)concat.data(), concat.size(),(unsigned char*)fullHash.data());
            numHashes++;
            bool found= checkLeadingZeroBits(fullHash, challengeSize);
            if (found) {
                SHA256Hash solution;
                memcpy(solution.data(),concat.data()+32,32);
                return solution;
            }
        }
    }

}

int main(int argc, char **argv) {
    double averageMiningTime = 0;
    int numTrials = 10;
    
    for (int j = 0; j < numTrials; j++) {
        long numHashes = 0;
        struct timeval time_now{};
        gettimeofday(&time_now, nullptr);
        time_t start = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
        benchmark::mineHashShifu(NULL_SHA256_HASH, 16, numHashes);
        gettimeofday(&time_now, nullptr);
        time_t end = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
        // time_t end = std::time(0);
        cout<<"Mined in : " <<(end-start)<<" ms"<<endl;
        cout<<"Hash Rate : " <<numHashes*1000/(end-start)<<endl;
        averageMiningTime += (end-start);
    }
    averageMiningTime/=numTrials;
    cout<<"Average Mining Time: "<<averageMiningTime<<endl;
}