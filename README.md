Welcome to PandaCoin! 
====================
<image src="https://github.com/mr-pandabear/panda-website/blob/master/site/static/logo.png" width="350"/>
PandaCoin is a minimalist implementation of a layer 1 cryptocurrency similar to Bitcoin. It is designed with utmost simplicity and user friendliness in mind and is written from the ground up in C++ — it isn't yet another re-packaging of existing open-source blockchain code (where's the fun in that?!). 

### Circulation
PandaCoin's are minted by miners who earn a reward of 50 bamboo per block. Mining payments occur up until block 2,000,000 after which miners are compensated purely through transaction fees. This means the total circulation of the currency is limited to 100,000,000 Bamboo.


### Technical Implementation
PandaCoin is written from the ground up in C++. We want the PandaCoin source code to be simple, elegant, and easy to understand. Rather than adding duct-tape to an existing currency, we built PandaCoin from scratch with lots of love. There are a few optimizations that we have made to help further our core objectives:
* Switched encryption scheme from [secp256k1](https://github.com/bitcoin-core/secp256k1) (which is used by ETH & BTC) to [ED25519](https://ed25519.cr.yp.to/) -- results in 8x speedup during verification and public keys half the size. 
* Transactions are created uniquely to each blockID
* Can queue transactions to execute up to 10 blocks in the future.
* 5000 transactions per block, 30 second block time
* Measured transactions per second > 400TPS

### API
* `GET: /block_count` : Returns the number of blocks in the current chain
* `GET: /stats` : Statistics on current chain in JSON format
* `GET: /block/<int:blockId>` : Block information in JSON format
* `GET: /ledger/<string:walletAddress>` : The balance for the specified wallet in JSON format
* `GET: /mine` : Returns the last block hash and difficulty in JSON format
* `GET: /sync/<int:startId>/<int:endId>` : Returns binary serialized blocks within range
* `GET: /synctx` : Returns binary serialized transactions in mem pool
* `POST: /submit` : Submits a new block (data must be binary serialized)
* `POST: /add_transaction` : Submits a new transaction (data must be binary serialized).

### Serialization formats
```
struct BlockHeader {
    int id;
    time_t timestamp;
    int difficulty;
    int numTransactions;
    char merkleRoot[32];
    char nonce[32];
};
```

```
struct TransactionInfo {
    int blockId;
    char signature[64];
    char signingKey[32];
    time_t timestamp;
    char nonce[8];
    char to[25];
    char from[25];
    long amount;
    long fee;
    bool isTransactionFee;
};

```


### Getting Started

### Building
Requires:
* CMake
* Conan package manager (http://www.conan.io)
* LevelDB
```
git clone https://github.com/mr-pandabear/panda-coin.git
cd panda-coin
git checkout stable
mkdir build
cd build
conan install ..
cd ..
cmake .
```
To compile the miner run the following command:
```
make miner
```
You will also need the keygen app to create a wallet for your miner:
```
make keygen
```

To compile the node server:
```
make server
```

### Usage
Start by generating `keys.json`. Keep a copy of this file -- it contains pub/private keys to the wallet that the miner will mint coins to.
```
./bin/keygen
```

To start mining:
```
./bin/miner
```

To host a node:
```
./bin/server
```







