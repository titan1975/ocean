#pragma once
#include "ILogger.hpp"
#include <curl/curl.h>
#include <memory>

class QuestDBLogger : public ILogger {
public:
    QuestDBLogger();
    ~QuestDBLogger() override;
    void log(const std::string& methodName, long durationMs) override;

private:
    CURL* curlHandle;
    void initializeCurl();
};