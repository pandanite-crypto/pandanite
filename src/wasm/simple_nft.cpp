#include <emscripten/emscripten.h>
#include <stdint.h>
#include <stdio.h>
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include "../core/crypto.hpp"
using namespace std;


#ifdef __cplusplus
extern "C" {
#endif
    #include "external_calls.hpp"
    void getInfo(const char* args) {
        
    }

    uint32_t executeBlock(const char* args) {
        char* ptr = (char*) args;
        BlockHeader blockH = blockHeaderFromBuffer(ptr);
        ptr += BLOCKHEADER_BUFFER_SIZE;
        vector<Transaction> transactions;
        for(int j = 0; j < blockH.numTransactions; j++) {
            TransactionInfo t = transactionInfoFromBuffer(ptr);
            ptr += transactionInfoBufferSize();
            Transaction tx(t);
            transactions.push_back(tx);
        }
        Block block(blockH, transactions);
        if (block.getId() == 1) {
            // generate the genesis record of NFT owner
            PublicWalletAddress ownerAddress = NULL_ADDRESS;
            SHA256Hash contentHash = NULL_SHA256_HASH;
            setWallet("owner", ownerAddress);
            setSha256("content", contentHash);
            return SUCCESS;
        } else {
            for(auto tx : block.getTransactions()) {
                PublicWalletAddress sender = tx.fromWallet();
                PublicWalletAddress recepient = tx.toWallet();
                PublicWalletAddress owner = getWallet("owner");
                if (owner == sender) {
                    if (!tx.signatureValid()) {
                        return INVALID_SIGNATURE;
                    }
                    // check the signature of the signing key
                    setWallet("owner", recepient);
                } else {
                    return BALANCE_TOO_LOW;
                }
            }
        }
        return 0;
    }

#ifdef __cplusplus
}
#endif
