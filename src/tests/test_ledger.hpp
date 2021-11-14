#include "../core/ledger.hpp"
#include "../core/crypto.hpp"
using namespace std;

TEST(test_ledger_stores_wallets) {
    std::pair<PublicKey,PrivateKey> pair = generateKeyPair();

    PublicWalletAddress wallet = walletAddressFromPublicKey(pair.first);

    Ledger ledger;
    ledger.init("./data/tmpdb");
    ledger.setWalletValue(wallet, 50.0);
    ASSERT_EQUAL(ledger.getWalletValue(wallet), 50.0);
}