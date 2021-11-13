Panda Coin
====================
![panda](https://upload.wikimedia.org/wikipedia/commons/thumb/b/b5/Noto_Emoji_KitKat_1f43c.svg/240px-Noto_Emoji_KitKat_1f43c.svg.png)

Welcome to PandaCoin! PandaCoin was designed with a simple philosophy: those that help create the global panda network should earn from the effort they put in, without having to sell off their coins.

To do this PandaCoin gives a certain portion of every transaction back to the user base, with a payout proportional to each founding wallets seniority, up to the first 1 million founding wallets. This means that the first 1 million users will always earn from the panda network because they believed in our currency early.

### Founding wallets require work
Unlike other cryptocurrencies, creating a PandaCoin founding wallet requires proof of work. This is to prevent users from simply creating thousands of fake wallets per person in order earn multiple rewards. Non-founding wallets can still transact on the network, but they will not earn any rewards.

### How it works
Wallet addresses correspond to public keys which are generated from private keys. The private key is a 256-bit number and you can generate as many of them as you want. We require founding wallet addresses to have a certain number of prefixed '0's for them to be admitted. The more '0's required the harder it is to find a wallet address that matches the pattern. We chose the difficulty level so that on an average modern laptop it will take roughly 2 hours to generate a founding wallet.

### Payouts
The total transaction founders fee is 0.5%. This 0.5% is split amongst three groups
- First 10,000 wallets    :  receive 0.5%
- First 100,000 wallets:  :  receive 0.5% * 2/3
- First 1,000,000 wallets :  receive 0.5% * 1/3

### Circulation
PandaCoin's are minted by miners who earn a reward of 50 bamboo per block. Mining payments occur up until block 200,000 after which miners are compensated purely through transaction fees. This means the total circulation of the currency is limited to 10,000,000 Bamboo.


### Technical Implementation
PandaCoin is written from the ground up in C++. We want the PandaCoin source code to be simple, elegant, and easy to understand. Rather than adding duct-tape to an existing currency, we build PandaCoin with love from the ground up. PandaCoin uses JSON rather than complex serialized formats to communicate between nodes. This is done for simplicity but we will eventually replace JSON for MessagePack once the blockcount reaches a certain threshold. 

Requires:
* CMake
* Conan package manager (http://www.conan.io)
* libsecp256k1 (https://github.com/bitcoin-core/secp256k1)

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







