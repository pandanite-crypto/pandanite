#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <list>
#include <ctime>
#include <mutex>

using namespace std;

#define TIME_FORMAT "%m-%d-%Y %H:%M:%S"
#define MAX_LINES 1000

#define RESET   std::string("\033[0m")
#define RED     std::string("\033[31m")
#define GREEN   std::string("\033[32m")

class Logger {
public:
    static list<string> buffer;
    static ofstream file;
    static std::mutex console_lock;
    static std::mutex file_mutex;
    static std::mutex buffer_mutex;

    static void logToBuffer(string message) {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        if (Logger::buffer.size() > MAX_LINES) {
            Logger::buffer.pop_front();
        }
        Logger::buffer.push_back(message);
    }

    static void logError(string endpoint, string message) {
        auto t = std::time(0);
        auto tm = *std::localtime(&t);
        stringstream s;
        s<<"[ERROR] "<<std::put_time(&tm, TIME_FORMAT)<<": "<<endpoint<<": "<<message<<endl;
        if (!file.is_open()) {
            std::unique_lock<std::mutex> ul(console_lock);
            cout << s.str() << std::flush;
        } else {
            std::lock_guard<std::mutex> lock(file_mutex);
            file<<s.str();
            file.flush();
        }
        Logger::logToBuffer(s.str());
    }

    static void logStatus(string message) {
        auto t = std::time(0);
        auto tm = *std::localtime(&t);
        stringstream s;
        s<<"[STATUS] "<<std::put_time(&tm, TIME_FORMAT)<<": "<<message<<endl;
        if (!file.is_open()) {
            std::unique_lock<std::mutex> ul(console_lock);
            cout << s.str() << std::flush;
        } else {
            std::lock_guard<std::mutex> lock(file_mutex);
            file<<s.str();
            file.flush();
        }  
        Logger::logToBuffer(s.str());
    }
};

//list<string> Logger::buffer = list<string>();
//ofstream Logger::file = ofstream();
//std::mutex Logger::console_lock;
//std::mutex Logger::file_mutex;
//std::mutex Logger::buffer_mutex;
