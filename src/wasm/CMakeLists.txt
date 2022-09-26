project(SimpleNFT)
cmake_minimum_required(VERSION 3.5.1)
option(JS_ONLY "Compiles to native JS (No WASM)" OFF)

set(CMAKE_C_COMPILER emcc)
set(CMAKE_CPP_COMPILER emcc)
set(CMAKE_CXX_COMPILER emcc)
set(HAVE_FLAG_SEARCH_PATHS_FIRST 0)
set(CMAKE_CXX_LINK_FLAGS "")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -c --target=wasm32-wasi -DWASM_BUILD  -DJSON_NO_IO -fexceptions -std=c++17")    


include_directories(../../include)
include_directories(../../lib)

file(GLOB CORE_SOURCES "../core/*.cpp")
file(GLOB EXTERNAL_SOURCES "../external/ed25519/*.c" "../external/bigint/*.cpp")

add_executable(simple_nft ${CORE_SOURCES} ${EXTERNAL_SOURCES} simple_nft.cpp )

target_link_libraries(simple_nft /Users/saliksyed/src/test-compile/test2/bamboo/src/wasm/libcrypto.a /Users/saliksyed/src/test-compile/test2/bamboo/src/wasm/libssl.a)


set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
set_target_properties(simple_nft PROPERTIES LINK_FLAGS  --no-entry -s NO_FILESYSTEM=1 -s FILESYSTEM=0 -s VERBOSE=1 -s STANDALONE_WASM -s ERROR_ON_UNDEFINED_SYMBOLS=0 -s EXPORTED_FUNCTIONS="['_executeBlock', '_getInfo']")


