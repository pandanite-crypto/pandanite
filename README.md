Welcome to PandaCoin! 
====================
<image src="https://github.com/mr-pandabear/panda-website/blob/master/site/static/logo.png" width="350"/>
PandaCoin is a minimalist implementation of a layer 1 blockchain protocol. It is designed with utmost simplicity and user friendliness in mind and is written from the ground up in C++ -- it isn't yet another re-packaging of existing open-source blockchain code (where's the fun in that?!). 

### PandaCoin rewards early users
PandaCoin is just getting started, and it's going to be quite a trek! We want to reward those that help create the PandaNet handsomely for their efforts. To do this PandaCoin gives 2.5% of every transaction back to the first 100,000 founding wallets. These payments last for the first 2 years that the network is live.

### Founding wallets require work
Unlike other cryptocurrencies, creating a PandaCoin founding wallet requires proof of work. This is to prevent users from simply creating thousands of founding wallets per person in order earn multiple rewards. Non-founding wallets can still transact on the network, but they will not earn any rewards.

### How it works
We require the SHA256 hash of founding wallet addresses (which are a function of public/private keys) to have a certain number of prefixed '0's for them to be admitted. The more '0's required the harder it is to find a wallet address that matches the pattern. We chose the difficulty level so that on an average modern laptop it will take roughly 2 hours to generate a founding wallet.

### Circulation
PandaCoin's are minted by miners who earn a reward of 50 bamboo per block. Mining payments occur up until block 2,000,000 after which miners are compensated purely through transaction fees. This means the total circulation of the currency is limited to 100,000,000 Bamboo.


### Technical Implementation
PandaCoin is written from the ground up in C++. We want the PandaCoin source code to be simple, elegant, and easy to understand. Rather than adding duct-tape to an existing currency, we built PandaCoin from scratch with lots of love. There are a few optimizations that we have made to help further our core objectives:
* Transactions created and signed on a per-block basis (no need for timestamping)
* Mempool that allows storage of transactions aimed at future blocks (improved queuing)
* 30 second block mint time (faster confirmations)
* Up to 1500 Transactions per block (more transactions per block)
* Ultra simple REST based interface

### Getting Started

### Building
Requires:
* CMake
* Conan package manager (http://www.conan.io)
* libsecp256k1 (https://github.com/bitcoin-core/secp256k1)
* LevelDB
```
git clone https://github.com/mrpandabear/panda-coin.git
cd panda-coin
mkdir build
cd build
conan install ..
cd ..
cmake .
make
```
### Libraries
* libsecp256k1
* uWebSockets
* JSON for modern CPP
* LevelDB
### Usage
The hosts list is stored hosts.txt. To start a server simply run:
```
./bin/server <port>
```

To start mining, generate a pub/private key as follows:
```
mkdir keys
./bin/cli
1 [add new user]
./keys/miner.json [enter the save file path]
```

This will create a file called miner.json that is read by the miner. You can run the miner as follows:
```
./bin/miner
```







