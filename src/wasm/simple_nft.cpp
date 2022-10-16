#include <emscripten/emscripten.h>
#include <stdint.h>
#include <stdio.h>
#include "../core/transaction.hpp"
#include "../core/block.hpp"
#include "../core/crypto.hpp"
#include "../core/types.hpp"
#include "../external/pufferfish/pufferfish.hpp"
#include "../external/ed25519/ed25519.h"
#include "../external/json.hpp"
#include "openssl/hmac.h"
using namespace std;


#ifdef __cplusplus
extern "C" {
#endif
    #include "external_calls.hpp"

    void getInfo() {
        print("getting info");
        PublicWalletAddress owner = getWallet("owner");
        string ret = "{ \"owner\": \"" + walletAddressToString(owner) + "\"}";
        const char* str = ret.c_str();
        print("returning");
        setReturnValue((char*) str, strlen(str) + 1);
    }

    void executeBlock(char* args) {
        char* ptr = args;
        print("starting");
        BlockHeader blockH = blockHeaderFromBuffer(ptr);
        ExecutionStatus e = SUCCESS;
        ptr += BLOCKHEADER_BUFFER_SIZE;
        vector<Transaction> transactions;
        for(int i = 0; i <  blockH.numTransactions; i++) {
            TransactionInfo t = transactionInfoFromBuffer(ptr, false);
            Transaction tx(t);
            transactions.push_back(tx);
            ptr += transactionInfoBufferSize(false);
        }
        print("read block");
        Block block(blockH, transactions);
        char str[1000];
        sprintf(str, "block ID %d", block.getId());
        print(str);
        if (block.getId() == 1) {
            print("genesis");
            // generate the genesis record of NFT owner
            PublicWalletAddress ownerAddress = stringToWalletAddress("00CF74F1F0111F43E305283219C9DF908D70A14A528083B77A");
            SHA256Hash contentHash = NULL_SHA256_HASH;
            setWallet("owner", ownerAddress);
            setSha256("content", contentHash);
            e = SUCCESS;
            setReturnValue((char*)&e, sizeof(ExecutionStatus));
            return;
        } else {
            e = SUCCESS;
            setReturnValue((char*)&e, sizeof(ExecutionStatus));
            for(auto tx : block.getTransactions()) {
                if (tx.isFee()) continue;
                print("Executing transaction");
                PublicWalletAddress sender = tx.fromWallet();
                PublicWalletAddress recepient = tx.toWallet();
                print("getting owner wallet");
                PublicWalletAddress owner = getWallet("owner");
                print("got owner wallet");
                print(walletAddressToString(sender).c_str());
                print(walletAddressToString(owner).c_str());
                if (owner == sender) {
                    print("sender is valid");
                    //check the signature of the signing key
                    if (!tx.signatureValid()) {
                        print("signature invalid");
                        e = INVALID_SIGNATURE;
                        setReturnValue((char*)&e, sizeof(ExecutionStatus));
                        return;
                    }
                    print("setting to new wallet");
                    setWallet("owner", recepient);
                    e = SUCCESS;
                    setReturnValue((char*)&e, sizeof(ExecutionStatus));
                } else {
                    print("sender is invalid");
                    e = BALANCE_TOO_LOW;
                    setReturnValue((char*)&e, sizeof(ExecutionStatus));
                    return;
                }
            }
        }
    }

#ifdef __cplusplus
}
#endif
