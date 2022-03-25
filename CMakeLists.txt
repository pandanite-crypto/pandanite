project(Bamboo)
cmake_minimum_required(VERSION 3.5.1)
option(JS_ONLY "Compiles to native JS (No WASM)" OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++17")    


include_directories(include)

file(GLOB CORE_SOURCES "src/core/*.cpp")
file(GLOB SERVER_SOURCES "src/server/*.cpp")
file(GLOB EXTERNAL_SOURCES "src/external/ed25519/*.c" "src/external/bigint/*.cpp")

add_executable(a ${CORE_SOURCES} ${SERVER_SOURCES} ${EXTERNAL_SOURCES} src/tools/server.cpp )

target_link_libraries(a ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcrypto.a ${CMAKE_CURRENT_SOURCE_DIR}/lib/libssl.a -sPROXY_TO_PTHREAD -sPTHREAD_POOL_SIZE=8 -sUSE_PTHREADS -sFETCH)

