 cmake_minimum_required(VERSION 2.8.12)
 project(Bamboo)

 
 if (WIN32)
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
 set(CONAN_DISABLE_CHECK_COMPILER TRUE)
else()
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")    
endif()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(${CMAKE_BINARY_DIR}/build/conanbuildinfo.cmake)
conan_basic_setup()

include_directories(/usr/local/include/)

file(GLOB CORE_SOURCES "src/core/*.cpp")
file(GLOB SERVER_SOURCES "src/server/*.cpp")
file(GLOB EXTERNAL_SOURCES "src/external/ed25519/*.c" "src/external/bigint/*.cpp")
add_executable(server ${CORE_SOURCES} ${SERVER_SOURCES} ${EXTERNAL_SOURCES} ./src/tools/server.cpp)
add_executable(tests ${CORE_SOURCES} ${SERVER_SOURCES} ${EXTERNAL_SOURCES} ./src/tests/tests.cpp)
target_link_libraries(tests ${CONAN_LIBS})
target_link_libraries(server ${CONAN_LIBS})

