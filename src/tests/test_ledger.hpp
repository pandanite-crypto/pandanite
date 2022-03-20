#include "../core/crypto.hpp"
#include "../server/ledger.hpp"
using namespace std;

TEST(test_ledger_stores_wallets) {
    std::pair<PublicKey,PrivateKey> pair = generateKeyPair();

    PublicWalletAddress wallet = walletAddressFromPublicKey(pair.first);

    Ledger ledger;
    ledger.createWallet(wallet);
    ledger.deposit(wallet, BMB(50.0));
    ASSERT_EQUAL(ledger.getWalletValue(wallet), BMB(50.0));
}