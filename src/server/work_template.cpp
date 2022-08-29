#include "work_template.hpp"
using namespace std;

typedef pair<int, Block> BlockEntry;
vector<BlockEntry> templates;

bool create_new_block(HostManager& hosts, Block& newBlock, PublicWalletAddress wallet)
{
    try {
        string host = hosts.getGoodHost();

        // fetch data
        json problem = getMiningProblem(host);
        int chainHeight = problem["chainLength"];
        int challengeSize = problem["challengeSize"];
        SHA256Hash lastHash = stringToSHA256(problem["lastHash"]);

        // download transactions
        vector<Transaction> transactions;
        readRawTransactions(host, transactions);

        // create fee to our wallet:
        Transaction fee(wallet, problem["miningFee"]);

        // set block id, fee and timestamp
        int nextBlock = chainHeight++;
        uint64_t lastTimestamp = (uint64_t)stringToUint64(problem["lastTimestamp"]);
        if (newBlock.getTimestamp() < lastTimestamp) {
            newBlock.setTimestamp(lastTimestamp + 1);
        }
        newBlock.setId(nextBlock);
        newBlock.addTransaction(fee);

        // add transactions
        TransactionAmount total = problem["miningFee"];
        if (newBlock.getTimestamp() < lastTimestamp) {
            newBlock.setTimestamp(lastTimestamp);
        }
        for (auto t : transactions) {
            newBlock.addTransaction(t);
            total += t.getTransactionFee();
        }

        // set block params
        MerkleTree m;
        m.setItems(newBlock.getTransactions());
        newBlock.setMerkleRoot(m.getRootHash());
        newBlock.setDifficulty(challengeSize);
        newBlock.setLastBlockHash(lastHash);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

int new_template_height(HostManager& hosts)
{
    string host = hosts.getGoodHost();
    json problem = getMiningProblem(host);
    int chainHeight = problem["chainLength"];
    return chainHeight++;
}

Block get_block_template(HostManager& hosts, PublicWalletAddress wallet)
{
    int search_height = new_template_height(hosts);

    // maintain cache
    if (templates.size() > 8) {
        templates.clear();
    }

    // check templates
    for (unsigned int i=0; i<templates.size(); i++) {
        if (templates.at(i).first == search_height) {
            return templates.at(i).second;
        }
    }

    Block newBlock;
    if (!create_new_block(hosts, newBlock, wallet)) {
        return Block();
    }

    BlockEntry newEntry{search_height, newBlock};
    templates.push_back(newEntry);

    return newBlock;
}

// blockhash = https://github.com/bamboo-crypto/bamboo/blob/master/src/core/block.cpp#L174-L184
// powhash   = https://github.com/bamboo-crypto/bamboo/blob/master/src/core/block.cpp#L152-L156

json get_work_template(HostManager& hosts, PublicWalletAddress wallet)
{
    Block newBlock = get_block_template(hosts, wallet);
    if (newBlock == Block()) {
        return json();
    }

    json tmpl;
    {
        json result;
        {
            result["height"] = newBlock.id;
            result["curtime"] = uint64ToString(newBlock.timestamp);
            result["previousblockhash"] = SHA256toString(newBlock.lastBlockHash, true);
            result["merkleRoot"] = SHA256toString(newBlock.merkleRoot, true);
            result["difficulty"] = newBlock.difficulty;
        }

        tmpl["id"] = 0;
        tmpl["error"] = "null";
        tmpl["result"] = result;
    }

    return tmpl;
}
