#pragma once

#include "transaction.hpp"
#include "constants.hpp"
#include "common.hpp"

class User {
    public:
        User();
        User(json u);
        json toJson();
        Transaction send(User & u, TransactionAmount amount);
        Transaction mine();
        PublicWalletAddress getAddress();
        PublicKey getPublicKey();
        PrivateKey getPrivateKey();
        void signTransaction(Transaction & t);
    protected:
        PrivateKey privateKey;
        PublicKey publicKey;
};