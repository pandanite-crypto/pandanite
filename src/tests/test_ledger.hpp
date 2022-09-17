#include "../core/crypto.hpp"
#include "../server/ledger.hpp"
using namespace std;

TEST(test_ledger_stores_wallets) {
    std::pair<PublicKey,PrivateKey> pair = generateKeyPair();

    PublicWalletAddress wallet = walletAddressFromPublicKey(pair.first);

    Ledger ledger;
    ledger.init("./test-data/tmpdb");
    ledger.createWallet(wallet);
    ledger.deposit(wallet, BMB(50.0));
    ASSERT_EQUAL(ledger.getWalletValue(wallet), BMB(50.0));
    ledger.closeDB();
    ledger.deleteDB();
}

TEST(test_ledger_stores_wallet_programs) {
    std::pair<PublicKey,PrivateKey> pair = generateKeyPair();

    PublicWalletAddress wallet = walletAddressFromPublicKey(pair.first);
    ProgramID programId = NULL_SHA256_HASH;
    Ledger ledger;
    ledger.init("./test-data/tmpdb2");
    ledger.createWallet(wallet);
    ASSERT_FALSE(ledger.hasWalletProgram(wallet));
    ledger.setWalletProgram(wallet, programId);
    ASSERT_EQUAL(ledger.getWalletProgram(wallet), programId);
    // ledger.removeWalletProgram(wallet);
    // ASSERT_FALSE(ledger.hasWalletProgram(wallet));
    // ledger.closeDB();
    // ledger.deleteDB();
}