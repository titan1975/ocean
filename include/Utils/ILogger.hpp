#pragma once
#include <string>

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(const std::string& methodName, long durationMs) = 0;
};
