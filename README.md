Bamboo 
====================
<image src="https://github.com/mr-pandabear/bamboo-utils/raw/master/logo.png" width=350/>
Bamboo is a minimalist implementation of a layer 1 cryptocurrency similar to Bitcoin. It is designed with utmost simplicity and user friendliness in mind and is written from the ground up in C++ — it isn't yet another re-packaging of existing open-source blockchain code (where's the fun in that?!). 

### Circulation
Bamboo is minted by miners who earn rewards. Mining payments occur using the following algorithm, which yields a total final circulation of 100M BMB
- 50 BMB per block until block 1M
- 25 BMB per block for blocks 1M to 2M
- 12.5 BMB per block for blocks 2M to 4M

### Technical Implementation
Bamboo is written from the ground up in C++. We want the Bamboo source code to be simple, elegant, and easy to understand. Rather than adding duct-tape to an existing currency, we built Bamboo from scratch with lots of love. There are a few optimizations that we have made to help further our core objectives:
* Switched encryption scheme from [secp256k1](https://github.com/bitcoin-core/secp256k1) (which is used by ETH & BTC) to [ED25519](https://ed25519.cr.yp.to/) -- results in 8x speedup during verification and public keys half the size. 
* Up to 25,000 transactions per block, 90 second block time

### Getting Started
*Windows*: Only binaries are available for windows they can be downloaded here:

https://github.com/mr-pandabear/bamboo/releases/download/v0.2-miner-alpha/bamboo-win-update.zip

- Run keygen.exe, this generates a keys.json file in the same folder as miner.exe, copy this file and keep it safe.
- Run miner.exe, the miner will start running and should indicate that it is loading blocks to solve.

*Mac OSX* build pre-requirements
```
brew install leveldb
brew install cmake
pip3 install conan
```


*Linux* install pre-requirements
```
sudo apt-get update
sudo apt-get -y install make cmake automake libtool python3-pip libleveldb-dev
sudo update-alternatives --install /usr/bin/python python /usr/bin/python3.6
sudo pip3 install conan
```

### Building
```
git clone https://github.com/mr-pandabear/bamboo.git
cd bamboo
git checkout release
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
Start by generating `keys.json`.

```
./bin/keygen
```
 ***Keep a copy of this file in a safe location*** -- it contains pub/private keys to the wallet that the miner will mint coins to. If you lose this file you lose your coins. We recommend keeping an extra copy on a unique thumbdrive (that you don't re-use) the moment you generate it.


To start mining:
```
./bin/miner
```

To host a node:
```
./bin/server
```
NOTE: you will need to make a folder `~/bamboo/data` to store the nodes data







