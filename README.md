panda-coin
====================
Simple C++ implementation of a blockchain based cryptocurrency

![panda](https://upload.wikimedia.org/wikipedia/commons/thumb/b/b5/Noto_Emoji_KitKat_1f43c.svg/240px-Noto_Emoji_KitKat_1f43c.svg.png)

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
To start, generate a pub/private key as follows:
```
mkdir keys
./bin/cli
1 [add new user]
./keys/miner.json [enter the save file path]
```
This will create a file called miner.json that is read by the miner. Once you have done this start a local chain server:
```
./bin/server 3000
```
You can now run the miner
```
./bin/miner
```
If everything is working you will see the miner successfully adding blocks.

Note that the difficulties setup here are very low. You'll want to look at `./src/core/constants.hpp` to put a larger transaction count, higher difficulty scores etc. You will need to edit the `genesis.json` file to reflect the min difficulty count.

If you want to run more server you can specify a host to mirror the blockchain from
```
./bin/server 8000 http://localhost:3000
```









