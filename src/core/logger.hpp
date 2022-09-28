#pragma once
#ifndef WASM_BUILD
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <ctime>
#include <mutex>
#endif
#include <string>
using namespace std;

#define TIME_FORMAT "%m-%d-%Y %H:%M:%S"
#define MAX_LINES 1000

#define RESET   std::string("\033[0m")
#define RED     std::string("\033[31m")
#define GREEN   std::string("\033[32m")

class Logger {
    public:
#ifndef WASM_BUILD
        static list<string> buffer;
        static ofstream file;
        inline static std::mutex console_lock;
#endif

        static void logToBuffer(string message) {
#ifndef WASM_BUILD
            if (Logger::buffer.size() > MAX_LINES) {
                Logger::buffer.pop_front();
            }
            Logger::buffer.push_back(message);
#endif
        }

        static void logError(string endpoint, string message) {
#ifndef WASM_BUILD
            auto t = std::time(0);
            auto tm = *std::localtime(&t);
            stringstream s;
            s<<"[ERROR] "<<std::put_time(&tm, TIME_FORMAT)<<": "<<endpoint<<": "<<message<<endl;
            if (!file.is_open()) {
                std::unique_lock<std::mutex> ul(Logger::console_lock);
                cout << s.str() << std::flush;
            } else {
                file<<s.str();
                file.flush();
            }
            Logger::logToBuffer(s.str());
#endif
        }

        static void logStatus(string message) {
#ifndef WASM_BUILD
            auto t = std::time(0);
            auto tm = *std::localtime(&t);
            stringstream s;
            s<<"[STATUS] "<<std::put_time(&tm, TIME_FORMAT)<<": "<<message<<endl;
            if (!file.is_open()) {
                std::unique_lock<std::mutex> ul(Logger::console_lock);
                cout << s.str() << std::flush;
            } else {
                file<<s.str();
                file.flush();
            }  
            Logger::logToBuffer(s.str());
#endif
        }
};
