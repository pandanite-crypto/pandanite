#include "../core/bloomfilter.hpp"
#include "../core/helpers.hpp"
#include <iostream>
#include <string>
#include <set>
using namespace std;

TEST(test_bloomfilter) {
    set<string> s;
    BloomFilter b(8000, 5);
    for(int i = 0; i < 1000; i++) {
        string r = randomString(32);
        s.insert(r);
        b.insert(r);
    }

    int numTruePositives = 0;
    int numFalseNegatives = 0;
    int numFalsePositives = 0;
    int numTrueNegatives = 0;
    for(auto r : s) {
        if(b.contains(r)) {
            numTruePositives += 1;
        } else {
            numFalseNegatives +=1;
        }
    }

    for (int i = 0; i < 1000; i++) {
        string r = randomString(5);
        if (b.contains(r)) {
            numFalsePositives += 1;
        } else {
            numTrueNegatives += 1;
        }
    }

    cout<<"True positives: "<< numTruePositives<<endl;
    cout<<"True negatives: "<< numTrueNegatives<<endl;
    cout<<"False positives: "<< numFalsePositives<<endl;
    cout<<"False negatives: "<< numFalseNegatives<<endl;

    char * serialized = b.serialize().first;
    BloomFilter c(serialized);
    ASSERT_TRUE(b==c);
    free(serialized);
}
