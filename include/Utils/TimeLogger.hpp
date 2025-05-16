#pragma once
#include <chrono>
#include <string>
#include "ILogger.hpp"

class TimeLogger {
public:
    explicit TimeLogger(ILogger& logger, std::string methodName);
    ~TimeLogger();

private:
    ILogger& logger;
    std::string methodName;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};