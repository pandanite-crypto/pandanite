/opt/wasi-sdk/bin/clang -O3 -nostdlib \
    -o simple_nft.wasm test.cpp \
    -Wl,--export=add -Wl,--export=__add \
    -Wl,--no-entry -Wl,--strip-all 