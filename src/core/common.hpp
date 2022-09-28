#pragma once
#ifndef WASM_BUILD
#include "../external/json.hpp"
#include "../external/bigint/bigint.h"
using namespace Dodecahedron;
using namespace nlohmann;
#endif
#include "openssl/sha.h"
#include "openssl/ripemd.h"
#include "types.hpp"
using namespace std;



string executionStatusAsString(ExecutionStatus s);
