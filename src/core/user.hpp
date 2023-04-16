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
        PublicWalletAddress getAddress() const;
        PublicKey getPublicKey() const;
        PrivateKey getPrivateKey() const;
        void signTransaction(Transaction & t);
    protected:
        PrivateKey privateKey;
        PublicKey publicKey;
};
