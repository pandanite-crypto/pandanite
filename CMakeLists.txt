cmake_minimum_required(VERSION 2.8.12)
project(Pandanite)

# Compiler Flags
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
    set(CONAN_DISABLE_CHECK_COMPILER TRUE)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")    
endif()

# Output directories
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Conan setup
include(${CMAKE_BINARY_DIR}/build/conanbuildinfo.cmake)
conan_basic_setup()

# Additional includes
include_directories(/usr/local/include/)

# Source files
file(GLOB CORE_SOURCES "src/core/*.cpp")
file(GLOB SERVER_SOURCES "src/server/*.cpp")
file(GLOB EXTERNAL_SOURCES "src/external/ed25519/*.c" "src/external/murmurhash3/*.cpp" "src/external/sha256/*.cpp" "src/external/bigint/*.cpp")


# Targets and linking
if (UNIX)
    add_executable(tests ${CORE_SOURCES} ${SERVER_SOURCES} ${EXTERNAL_SOURCES} ./src/tests/tests.cpp)
    add_executable(server ${CORE_SOURCES} ${SERVER_SOURCES} ${EXTERNAL_SOURCES} ./src/tools/server.cpp)
    add_executable(cli ${CORE_SOURCES} ${SERVER_SOURCES} ${EXTERNAL_SOURCES} ./src/tools/cli.cpp)
    add_executable(loader ${CORE_SOURCES} ${SERVER_SOURCES} ${EXTERNAL_SOURCES} ./src/tools/loader.cpp)
    add_executable(tx ${CORE_SOURCES} ${SERVER_SOURCES}  ${EXTERNAL_SOURCES} ./src/tools/tx.cpp)
    add_executable(miner ${CORE_SOURCES} ${SERVER_SOURCES} ${EXTERNAL_SOURCES} ./src/tools/miner.cpp)
    add_executable(keygen ${CORE_SOURCES} ${SERVER_SOURCES} ${EXTERNAL_SOURCES} ./src/tools/keygen.cpp)
    
    if (APPLE)
        # Link libraries for APPLE
        foreach(TARGET tests server cli loader tx miner keygen)
            target_link_libraries(${TARGET} ${CONAN_LIBS} /usr/local/Cellar/leveldb/1.23/lib/libleveldb.a /usr/local/Cellar/snappy/1.1.9/lib/libsnappy.a)
        endforeach()
    endif()

    if (UNIX AND NOT APPLE)
        # Link libraries for UNIX
        foreach(TARGET tests server cli loader tx miner keygen)
            target_link_libraries(${TARGET} ${CONAN_LIBS} -lleveldb -lstdc++fs -lssl -lcrypto)
        endforeach()
    endif()
endif()
