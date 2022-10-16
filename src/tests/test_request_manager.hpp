#include "../shared/user.hpp"
#include "../core/block.hpp"
#include "../core/merkle_tree.hpp"
#include "../core/transaction.hpp"
#include "../shared/host_manager.hpp"
#include "../shared/config.hpp"
#include "../server/request_manager.hpp"
#include <thread>
using namespace std;

json config;


TEST(test_accepts_proof_of_work) {
    HostManager hosts;
    json config = getConfig(0,NULL);
    config["storagePath"] = "./test-data/prog_";
    config["mockHosts"] = true;
    RequestManager r(config);
    json pow = r.getProofOfWork();
    string lastHashStr = pow["lastHash"];
    SHA256Hash lastHash = stringToSHA256(lastHashStr);
    int challengeSize = pow["challengeSize"];
    
    User miner;
    // have miner mine the next block
    Transaction fee = miner.mine();
    vector<Transaction> transactions;
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    MerkleTree m;
    m.setItems(newBlock.getTransactions());
    newBlock.setMerkleRoot(m.getRootHash());
    newBlock.setLastBlockHash(lastHash);
    SHA256Hash hash = newBlock.getHash();
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    json result = r.submitProofOfWork(newBlock);
    std::cout<<result.dump()<<std::endl;
    ASSERT_EQUAL(result["status"], "SUCCESS");
    //r.deleteDB();
}

TEST(test_fails_when_missing_merkle_root) {
    HostManager hosts;
    json config = getConfig(0,NULL);
    config["storagePath"] = "./test-data/prog_";
    config["mockHosts"] = true;
    RequestManager r(config);
    json pow = r.getProofOfWork();
    string lastHashStr = pow["lastHash"];
    SHA256Hash lastHash = stringToSHA256(lastHashStr);
    int challengeSize = pow["challengeSize"];
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    User miner;
    // have miner mine the next block
    Transaction fee = miner.mine();
    vector<Transaction> transactions;
    Block newBlock;
    newBlock.setId(2);
    newBlock.addTransaction(fee);
    newBlock.setLastBlockHash(lastHash);
    SHA256Hash hash = newBlock.getHash();
    SHA256Hash solution = mineHash(hash, newBlock.getDifficulty());
    newBlock.setNonce(solution);
    json result = r.submitProofOfWork(newBlock);
    //r.deleteDB();
    ASSERT_EQUAL(result["status"], "INVALID_MERKLE_ROOT");
}