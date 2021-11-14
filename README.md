Welcome to PandaCoin! 
====================
<image src="https://github.com/mr-pandabear/panda-website/blob/master/site/static/logo.png" width="350"/>
PandaCoin was designed with a simple philosophy: those that help create the global panda network should earn from the effort they put in, without having to sell off their coins.

To do this PandaCoin gives a certain portion of every transaction back to the user base, with a payout proportional to each founding wallets seniority, up to the first 1 million founding wallets. This means that the first 1 million users will always earn from the panda network because they believed in our currency early.

### Founding wallets require work
Unlike other cryptocurrencies, creating a PandaCoin founding wallet requires proof of work. This is to prevent users from simply creating thousands of fake wallets per person in order earn multiple rewards. Non-founding wallets can still transact on the network, but they will not earn any rewards.

### How it works
Wallet addresses correspond to public keys which are generated from private keys. The private key is a 256-bit number and you can generate as many of them as you want. We require the SHA256 hash of founding wallet addresses to have a certain number of prefixed '0's for them to be admitted. The more '0's required the harder it is to find a wallet address that matches the pattern. We chose the difficulty level so that on an average modern laptop it will take roughly 2 hours to generate a founding wallet.

### Payouts
The total transaction founders fee is 0.5%. This 0.5% is split amongst three groups
- First 10,000 wallets    :  receive 0.5%
- First 100,000 wallets:  :  receive 0.5% * 2/3
- First 1,000,000 wallets :  receive 0.5% * 1/3

### Circulation
PandaCoin's are minted by miners who earn a reward of 50 bamboo per block. Mining payments occur up until block 200,000 after which miners are compensated purely through transaction fees. This means the total circulation of the currency is limited to 10,000,000 Bamboo.


### Technical Implementation
PandaCoin is written from the ground up in C++. We want the PandaCoin source code to be simple, elegant, and easy to understand. Rather than adding duct-tape to an existing currency, we built PandaCoin from the ground up with lots of love/

### Getting Started

1. The first step is to create a founding wallet. This can be done using the create wallet application. Save the key file and keep it in a safe location. Never share it with anyone. If you lose this key you loose your wallet.
2. The second step is to setup a node. Nodes can have mining switched on or switched off (if you don't want the node hogging your CPU). Instructions for compiling the code from source are below. We also offer hosted panda coin miner nodes for $49.99 a month (to pay for our server costs).
3. Once the node is ready, you can start it by specifying your public wallet address (which is written in the key-file). This address is where coins mined by the node will be stored. You'll also want port 3000 to be open to allow for communication to the node. Once your node is running and stable, send us an email at canihavebamboo@gmail.com and we will add you to the node list.
4. If you'd like to get a hosted node, or have trouble with the instructions, feel free to contact us at canihavebamboo@gmail.com or create a github issue.

### Building
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







