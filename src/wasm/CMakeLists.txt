project(wasm_modules)
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --target=wasm32-wasi --no-entry -DWASM_BUILD -DJSON_NO_IO  -std=c++17")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --target=wasm32-wasi")
include_directories(../../include)

file(GLOB CORE_SOURCES "../core/*.cpp")
file(GLOB EXTERNAL_SOURCES "../external/ed25519/*.c"  "../external/bigint/*.cpp" "../external/pufferfish/*.cpp")


add_executable(test_basic  test_basic.cpp ${CORE_SOURCES} ${EXTERNAL_SOURCES})
target_compile_definitions(test_basic PRIVATE WASM_BUILD=1 )
set_target_properties(test_basic PROPERTIES LINK_FLAGS "-s --no-entry -s STANDALONE_WASM=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -s EXPORTED_FUNCTIONS=\"['_executeBlock']\"")
target_link_libraries(test_basic -Wl,--allow-undefined ${CMAKE_CURRENT_SOURCE_DIR}/libssl.a ${CMAKE_CURRENT_SOURCE_DIR}/libcrypto.a -stdlib)

add_executable(simple_nft  simple_nft.cpp ${CORE_SOURCES} ${EXTERNAL_SOURCES})
target_compile_definitions(simple_nft PRIVATE WASM_BUILD=1 )
set_target_properties(simple_nft PROPERTIES LINK_FLAGS "-s --no-entry -s STANDALONE_WASM=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -s EXPORTED_FUNCTIONS=\"['_executeBlock']\"")
target_link_libraries(simple_nft -Wl,--allow-undefined ${CMAKE_CURRENT_SOURCE_DIR}/libssl.a ${CMAKE_CURRENT_SOURCE_DIR}/libcrypto.a -stdlib)




set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
