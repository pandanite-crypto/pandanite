#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
using namespace std;

#define TIME_FORMAT "%m-%d-%Y %H:%M:%S"

class Logger {
    public:
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
            
        }
        static ofstream file;
};
