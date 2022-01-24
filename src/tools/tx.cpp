#include "../core/common.hpp"
#include "../core/crypto.hpp"
#include "../core/helpers.hpp"
#include "../core/transaction.hpp"
using namespace std;

int main(int argc, char** argv) {
    if (argc != 7) {
        cout<<"Usage: ./bin/tx <publicKey> <privateKey> <toAddress> <amount(uint64_t)> <fee(uint64_t)> <nonce(uint64_t)> "<<endl;
        cout<<"e.g ./bin/tx 827A1052F97B16C25E91837D61EAD7C4EB783F314DD065CA87E40A51A43FE664 D0924F5DFDD763DD94E0C31727FEA17056269050C28D307261E3BE36F17D2B7175C00648ACF96DA5E0DD30D092BD136824F1DCA90A713971495E422A257DB115 005D890F7334CEBAE98AFDDA810DA9D656EE698B17CDD49212 500000 0 1"<<endl;
        return 0;
    }
    PublicKey publicKey = stringToPublicKey(string(argv[1]));
    PrivateKey privateKey = stringToPrivateKey(string(argv[2]));    
    PublicWalletAddress toAddress = stringToWalletAddress(string(argv[3]));
    PublicWalletAddress fromAddress = walletAddressFromPublicKey(publicKey);
    TransactionAmount amount = stringToUint64(argv[4]);
    TransactionAmount fee = stringToUint64(argv[5]);
    uint64_t nonce = stringToUint64(argv[6]);

    Transaction t(fromAddress, toAddress,amount, publicKey, fee, nonce);
    t.sign(publicKey, privateKey);
    cout<<t.toJson().dump()<<endl;
    return 0;
}