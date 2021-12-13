#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <list>
#include <ctime>
using namespace std;

#define TIME_FORMAT "%m-%d-%Y %H:%M:%S"
#define MAX_LINES 1000

class Logger {
    public:
        static list<string> buffer;
        static ofstream file;
        
        static void logToBuffer(string message) {
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
                cout<<s.str();
            } else {
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
                cout<<s.str();
            } else {
                file<<s.str();
                file.flush();
            }  
            Logger::logToBuffer(s.str());
        }
};
