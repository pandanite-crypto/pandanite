project(Bamboo)
cmake_minimum_required(VERSION 3.5.1)
option(JS_ONLY "Compiles to native JS (No WASM)" OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -pthread -std=c++17")    


include_directories(include)

file(GLOB CORE_SOURCES "src/core/*.cpp")
file(GLOB SERVER_SOURCES "src/server/*.cpp")
file(GLOB EXTERNAL_SOURCES "src/external/ed25519/*.c" "src/external/bigint/*.cpp")

add_executable(a ${CORE_SOURCES} ${SERVER_SOURCES} ${EXTERNAL_SOURCES} src/tools/server.cpp )

target_link_libraries(a ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcrypto.a ${CMAKE_CURRENT_SOURCE_DIR}/lib/libssl.a  -fexceptions -sNO_DISABLE_EXCEPTION_CATCHING -sINITIAL_MEMORY=167772160 -sPROXY_TO_PTHREAD -sPTHREAD_POOL_SIZE=8 -sUSE_PTHREADS -sFETCH)

if(JS_ONLY)
	message(STATUS "Setting compilation target to native JavaScript")
	set(CMAKE_EXECUTABLE_SUFFIX ".js")
	set_target_properties(a PROPERTIES LINK_FLAGS "-s 'EXPORTED_RUNTIME_METHODS=[\"ccall\", \"cwrap\", \"intArrayFromString\", \"allocate\"]' -s WASM=0 -s EXPORTED_FUNCTIONS='[_main]'")
else(JS_ONLY)
	message(STATUS "Setting compilation target to WASM")
	set(CMAKE_EXECUTABLE_SUFFIX ".wasm.js")
	set_target_properties(a PROPERTIES LINK_FLAGS "-s 'EXPORTED_RUNTIME_METHODS=[\"ccall\", \"cwrap\", \"intArrayFromString\", \"allocate\"]' -s WASM=1 -s EXPORTED_FUNCTIONS='[_main]'")
endif(JS_ONLY)

