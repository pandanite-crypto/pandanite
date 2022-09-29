#pragma once
#ifndef WASM_BUILD
#include "../external/json.hpp"
using namespace nlohmann;
#endif
#include "openssl/sha.h"
#include "openssl/ripemd.h"
#include "types.hpp"
#include "../external/bigint/bigint.h"
using namespace Dodecahedron;
using namespace std;



string executionStatusAsString(ExecutionStatus s);
