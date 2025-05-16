#include "Utils/QuestDBLogger.hpp"
#include <stdexcept>
#include <curl/curl.h>

QuestDBLogger::QuestDBLogger() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curlHandle = curl_easy_init();
    if (!curlHandle) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

QuestDBLogger::~QuestDBLogger() {
    if (curlHandle) curl_easy_cleanup(curlHandle);
    curl_global_cleanup();
}

void QuestDBLogger::log(const std::string& methodName, long durationMs) {
    const std::string query =
        "INSERT INTO execution_times(ts, methodName, durationMs) "
        "VALUES(systimestamp(), '" + methodName + "', " + std::to_string(durationMs) + ")";

    char* escaped = curl_easy_escape(curlHandle, query.c_str(), query.length());
    if (!escaped) return;

    const std::string url = "http://localhost:9000/exec?query=" + std::string(escaped);
    curl_free(escaped);

    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, [](void*, size_t size, size_t nmemb, void*) {
        return size * nmemb;
    });

    CURLcode res = curl_easy_perform(curlHandle);
    if (res != CURLE_OK) {
        // Handle error
    }
}