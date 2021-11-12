#include "../core/request_manager.hpp"
#include "../core/user.hpp"
#include "../core/block.hpp"
#include "../core/transaction.hpp"
using namespace std;

TEST(test_accepts_proof_of_work) {
    RequestManager r;

    json pow = r.getProofOfWork();
    string lastHashStr = pow["lastHash"];
    SHA256Hash lastHash = stringToSHA256(lastHashStr);
    int challengeSize = pow["challengeSize"];
    
    User miner;
    // have miner mine the next block
    Transaction fee = miner.mine(2);
    vector<Transaction> transactions;
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    SHA256Hash hash = newBlock.getHash(lastHash);
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    json submission;
    submission["block"] = newBlock.toJson();
    json result = r.submitProofOfWork(submission);
    ASSERT_EQUAL(result["status"], "SUCCESS");
}