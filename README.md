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







