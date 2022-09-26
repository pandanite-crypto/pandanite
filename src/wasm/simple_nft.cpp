#include <stdint.h>
#include <emscripten/emscripten.h>
// #include "../external/json.hpp"
#include "../core/types.hpp"
// #include "../core/transaction.hpp"
// #include "../core/block.hpp"
// #include "../core/crypto.hpp"

extern "C" {
    // #include "external_calls.hpp"

    void getInfo(const char* args) {
        // json req = json::parse(string(args));
        // json ret;
        // ret["owner"] = walletAddressToString(getWallet("owner"));
        // string output = ret.dump();
        // setReturnValue(output.c_str());
    }

    ExecutionStatus executeBlock(const char* args) {
        // Block block = getBlock(args);
        // // parse data into block header + transactions
        // if (block.getId() == 1) {
        //     // generate the genesis record of NFT owner
        //     PublicWalletAddress ownerAddress = NULL_ADDRESS;
        //     SHA256Hash contentHash = NULL_SHA256_HASH;
        //     setWallet("owner", ownerAddress);
        //     setSha256("content", contentHash);
        //     return SUCCESS;
        // } else {
        //     for(auto tx : block.getTransactions()) {
        //         PublicWalletAddress sender = tx.fromWallet();
        //         PublicWalletAddress recepient = tx.toWallet();
        //         PublicWalletAddress owner = getWallet("owner");
        //         if (owner == sender) {
        //             if (!tx.signatureValid()) {
        //                 return INVALID_SIGNATURE;
        //             }
        //             // check the signature of the signing key
        //             setWallet("owner", recepient);
        //         } else {
        //             return BALANCE_TOO_LOW;
        //         }
        //     }
        // }
        return SUCCESS;
    }
}