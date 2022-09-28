#include "logger.hpp"
#ifndef WASM_BUILD
ofstream Logger::file = ofstream();

list<string> Logger::buffer = list<string>();
#endif