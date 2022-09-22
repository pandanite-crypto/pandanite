#pragma once
#include "../external/json.hpp"
#include "../external/bigint/bigint.h"
#include "openssl/sha.h"
#include "openssl/ripemd.h"
#include "types.hpp"
using namespace Dodecahedron;
using namespace std;
using namespace nlohmann;


string executionStatusAsString(ExecutionStatus s);
