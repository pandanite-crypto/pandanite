#include "logger.hpp"

ofstream Logger::file = ofstream();

list<string> Logger::buffer = list<string>();