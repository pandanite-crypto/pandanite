 emcc --std=c++17 -o simple_nft.wasm simple_nft.cpp \
    --no-entry  -s EXPORTED_FUNCTIONS="['_executeBlock', '_getInfo']" \
    -s STANDALONE_WASM -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
    -I../../include 
