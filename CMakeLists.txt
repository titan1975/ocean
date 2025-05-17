cmake_minimum_required(VERSION 3.20)
project(Ocean LANGUAGES CXX)

# ================== COMPILER CONFIG ==================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ================== GCC 14 CONFIG ==================
set(GCC14_LIB_PATH "/opt/gcc-14.1.0/lib64")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${GCC14_LIB_PATH}")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${GCC14_LIB_PATH}/cmake")

# ================== NUCLEAR HEADER SOLUTION ==================
set(OCEAN_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(OCEAN_SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Force CLion to recognize ALL headers
file(GLOB_RECURSE OCEAN_HEADERS "${OCEAN_INCLUDE_DIR}/*.hpp" "${OCEAN_INCLUDE_DIR}/*.h")
add_custom_target(OCEAN_HEADERS_TARGET SOURCES ${OCEAN_HEADERS})

# ================== DEPENDENCIES ==================
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(simdjson REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(ZLIB REQUIRED)

# ================== CORE LIBRARY ==================
add_library(OceanCore STATIC
        ${OCEAN_SRC_DIR}/Analysis/MarketPhaseDetector.cpp
        ${OCEAN_SRC_DIR}/Core/MarketData.cpp
        ${OCEAN_SRC_DIR}/Core/OrderBook.cpp
        ${OCEAN_SRC_DIR}/Risk/RiskManager.cpp
        ${OCEAN_SRC_DIR}/Strategy/GammaSqueezeDetector.cpp
        ${OCEAN_SRC_DIR}/Strategy/LiquidityDetector.cpp
        ${OCEAN_SRC_DIR}/Strategy/LiquidityRaidDetector.cpp
        ${OCEAN_SRC_DIR}/Tactics/SunTzuTactics.cpp
        ${OCEAN_SRC_DIR}/Utils/QuestDBLogger.cpp
        ${OCEAN_SRC_DIR}/Utils/TimeLogger.cpp
)

target_include_directories(OceanCore PUBLIC
        ${OCEAN_INCLUDE_DIR}
)

target_link_libraries(OceanCore PRIVATE
        ZLIB::ZLIB
        spdlog::spdlog
        fmt::fmt
        ${ZMQ_LIBRARIES}
)

# ================== MAIN EXECUTABLE ==================
add_executable(OceanMain ${OCEAN_SRC_DIR}/main.cpp)

target_link_libraries(OceanMain PRIVATE
        OceanCore
        ${CURL_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        simdjson::simdjson
)

# ================== TESTS ==================
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

#add_executable(OceanTests
      #  ${OCEAN_SRC_DIR}/tests/TestQuestDBLogger.cpp
       # ${OCEAN_SRC_DIR}/tests/TestLiquidityDetector.cpp
#)

#target_link_libraries(OceanTests PRIVATE
#       OceanCore
#       GTest::gtest_main
#       ${CURL_LIBRARIES}
#)

# ================== COMPILER OPTIMIZATIONS ==================
target_compile_options(OceanCore PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3
        -march=native
)

# ================== INSTALL ==================
install(TARGETS OceanMain DESTINATION bin)
install(TARGETS OceanCore ARCHIVE DESTINATION lib)