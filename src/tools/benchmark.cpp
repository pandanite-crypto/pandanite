#include "../core/executor.hpp"
#include "../core/user.hpp"
#include "../core/crypto.hpp"
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include <iostream>
using namespace std;


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
        ledger.init("./data/tmpdb");
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