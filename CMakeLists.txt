 cmake_minimum_required(VERSION 2.8.12)
 project(SimpleCoin)

 add_definitions("-std=c++17")

 include(${CMAKE_BINARY_DIR}/build/conanbuildinfo.cmake)
 conan_basic_setup()

include_directories(/usr/local/include/)

file(GLOB CORE_SOURCES "src/core/*.cpp")
 add_executable(tests ${CORE_SOURCES} ./src/tests/tests.cpp)
 add_executable(server ${CORE_SOURCES} ./src/tools/server.cpp)
 add_executable(cli ${CORE_SOURCES} ./src/tools/cli.cpp)
 add_executable(miner ${CORE_SOURCES} ./src/tools/miner.cpp)
 if (APPLE)
    target_link_libraries(tests ${CONAN_LIBS} /usr/local/lib/libsecp256k1.0.dylib)
    target_link_libraries(server ${CONAN_LIBS} /usr/local/lib/libsecp256k1.0.dylib)
    target_link_libraries(cli ${CONAN_LIBS} /usr/local/lib/libsecp256k1.0.dylib)
    target_link_libraries(miner ${CONAN_LIBS} /usr/local/lib/libsecp256k1.0.dylib)
endif()

if (UNIX)
    target_link_libraries(tests ${CONAN_LIBS} /usr/local/lib/libsecp256k1.so)
    target_link_libraries(server ${CONAN_LIBS} /usr/local/lib/libsecp256k1.so)
    target_link_libraries(cli ${CONAN_LIBS} /usr/local/lib/libsecp256k1.so)
    target_link_libraries(miner ${CONAN_LIBS} /usr/local/lib/libsecp256k1.so)
endif()