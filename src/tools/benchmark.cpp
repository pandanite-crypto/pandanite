#include "../core/executor.hpp"
#include "../core/user.hpp"
#include "../core/crypto.hpp"
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include <iostream>
#include <ctime>
using namespace std;



SHA256Hash concatHashes(SHA256Hash a, SHA256Hash b) {
    vector<uint8_t> concat;
    for(size_t i = 0; i < a.size(); i++) {
        concat.push_back(a[i]);
    }
    for(size_t i = 0; i < b.size(); i++) {
        concat.push_back(b[i]);
    }
    SHA256Hash fullHash  = SHA256((const char*)concat.data(), concat.size());
    return fullHash;
}

bool checkLeadingZeroBits(SHA256Hash hash, unsigned int challengeSize) {
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


bool verifyHash(SHA256Hash target, SHA256Hash nonce, unsigned char challengeSize) {
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


int main(int argc, char **argv) {
    double averageValidationTime = 0;
    double averageCreationTime = 0;
    int numTrials = 10;
    for (int j = 0; j < numTrials; j++) {
        User miner;
        vector<User> randomUsers;
        for(int i = 0; i < 40; i++) {
            User u;
            randomUsers.push_back(u);
        }
        time_t start = std::time(0);
        Block b;
        Transaction fee = miner.mine(1);
        b.addTransaction(fee);
        for(int i = 0; i < 1500; i++) {
            User r = randomUsers[rand()%randomUsers.size()];
            Transaction t = miner.send(r, 1 + rand()%5, 1);
            b.addTransaction(t);
        }
        time_t end = std::time(0);
        averageCreationTime += (end-start);
        cout<<"Time to generate 1500 transactions: " << end-start << " seconds"<<endl;
        LedgerState deltas;
        Ledger ledger;
        ledger.init("./test-data/tmpdb");
        start = std::time(0);
        ExecutionStatus status = Executor::ExecuteBlock(b, ledger, deltas);
        cout<<"Status: " << executionStatusAsString(status) <<endl;
        end = std::time(0);
        averageValidationTime += (end-start);
        cout<<"Time to validate 1500 transactions: " << end-start << " seconds"<<endl;
    }
    averageCreationTime/=numTrials;
    averageValidationTime/=numTrials;
    cout<<"Average Creation Time: "<<averageCreationTime<<endl;
    cout<<"Average Validation Time: "<<averageValidationTime<<endl;
}