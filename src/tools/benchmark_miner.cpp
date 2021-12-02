#include "../core/common.hpp"
#include "../core/crypto.hpp"
#include <iostream>
#include <ctime>
using namespace std;

namespace benchmark {

    SHA256Hash concatHashes(SHA256Hash& a, SHA256Hash& b) {
        char data[64];
        memcpy(data, (char*)a.data(), 32);
        memcpy(&data[32], (char*)b.data(), 32);
        SHA256Hash fullHash  = SHA256((const char*)data, 64);
        return fullHash;
    }

    bool checkLeadingZeroBits(SHA256Hash& hash, unsigned int challengeSize) {
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


    bool verifyHash(SHA256Hash& target, SHA256Hash& nonce, unsigned char challengeSize) {
        SHA256Hash fullHash  = concatHashes(target, nonce);
        return checkLeadingZeroBits(fullHash, challengeSize);
    }

    SHA256Hash mineHash(SHA256Hash targetHash, unsigned char challengeSize) {
        while(true) {
            SHA256Hash solution;
            for(size_t i = 0; i < solution.size(); i++) {
                solution[i] = rand()%256;
            }
            bool found = verifyHash(targetHash, solution, challengeSize);
            if (found) {
                return solution;
            }
        }
    }
}

int main(int argc, char **argv) {
    double averageMiningTime = 0;
    int numTrials = 10;
    for (int j = 0; j < numTrials; j++) {
        time_t start = std::time(0);
        benchmark::mineHash(NULL_SHA256_HASH, 18);
        time_t end = std::time(0);
        cout<<"Mined in : " <<(end-start)<<endl;
        averageMiningTime += (end-start);
    }
    averageMiningTime/=numTrials;
    cout<<"Average Mining Time: "<<averageMiningTime<<endl;
}