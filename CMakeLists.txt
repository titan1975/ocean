cmake_minimum_required(VERSION 3.20)
project(Ocean LANGUAGES CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GCC 14 specific settings
set(GCC14_LIB_PATH "/opt/gcc-14.1.0/lib64")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${GCC14_LIB_PATH}")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${GCC14_LIB_PATH}/cmake")



# Dependencies
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(simdjson REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(ZLIB REQUIRED)

# Include directories
include_directories(include)

# Main library
add_library(OceanCore STATIC
        src/Core/OrderBook.cpp
        src/Core/MarketData.cpp
        src/Strategy/LiquidityRaidDetector.cpp
        src/Strategy/GammaSqueezeDetector.cpp
        src/Utils/QuestDBLogger.cpp
        src/Utils/TimeLogger.cpp
        src/Strategy/LiquidityDetector.cpp
        include/Risk/RiskManager.hpp
        src/Risk/RiskManager.cpp
        src/Tactics/SunTzuTactics.cpp
        include/Detectors/MarketPhaseDetector.hpp
)


target_link_libraries(OceanCore PRIVATE ZLIB::ZLIB)


# Main executable
add_executable(OceanMain src/main.cpp
        include/Tactics/SunTzuTactics.hpp)

target_link_libraries(OceanMain PRIVATE
        OceanCore
        ${CURL_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        simdjson::simdjson
        ${ZMQ_LIBRARIES}
)

# Tests
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_executable(OceanTests
        tests/TestQuestDBLogger.cpp
        tests/TestLiquidityDetector.cpp
)

target_link_libraries(OceanTests PRIVATE
        OceanCore
        GTest::gtest_main
        ${CURL_LIBRARIES}
)

# Compiler options
target_compile_options(OceanCore PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3
        -march=native
)

# Install
install(TARGETS OceanMain DESTINATION bin)
install(TARGETS OceanCore ARCHIVE DESTINATION lib)