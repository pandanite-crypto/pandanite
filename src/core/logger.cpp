#include "logger.hpp"

list<string> Logger::buffer = list<string>();
ofstream Logger::file = ofstream();
std::mutex Logger::console_lock;
std::mutex Logger::file_mutex;
std::mutex Logger::buffer_mutex;
