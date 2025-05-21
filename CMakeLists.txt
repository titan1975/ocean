cmake_minimum_required(VERSION 3.20)
project(Ocean LANGUAGES CXX)

# ================== COMPILER CONFIG ==================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-O3 -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
# ================== CMP0167 POLICY FOR BOOST ==================
cmake_policy(SET CMP0167 NEW)

# ================== GCC 14 CONFIG ==================
set(GCC14_LIB_PATH "/opt/gcc-14.1.0/lib64")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${GCC14_LIB_PATH}")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${GCC14_LIB_PATH}/cmake")

# ================== DIRECTORIES ==================
set(OCEAN_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(OCEAN_SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Force CLion to recognize headers
file(GLOB_RECURSE OCEAN_HEADERS "${OCEAN_INCLUDE_DIR}/*.hpp" "${OCEAN_INCLUDE_DIR}/*.h")
add_custom_target(OCEAN_HEADERS_TARGET SOURCES ${OCEAN_HEADERS})

# ================== DEPENDENCIES ==================
find_package(CURL REQUIRED)
set(OPENSSL_ROOT_DIR "/usr/lib/x86_64-linux-gnu")
find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(simdjson REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(ZLIB REQUIRED)
find_package(Boost REQUIRED)

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
        src/Clients/BinanceWSClient.cpp
        include/Clients/BinanceWSClient.hpp
)

target_include_directories(OceanCore PUBLIC
        ${OCEAN_INCLUDE_DIR}
)

target_link_libraries(OceanCore PRIVATE
        ZLIB::ZLIB
        spdlog::spdlog
        fmt::fmt
        ${ZMQ_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        Boost::boost
)

# Compile options and features for OceanCore
target_compile_options(OceanCore PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3
        -march=native
)
target_compile_features(OceanCore PRIVATE cxx_std_23)

# ================== MAIN EXECUTABLE ==================
add_executable(OceanMain ${OCEAN_SRC_DIR}/main.cpp
        src/Clients/BinanceWSClient.cpp
        include/Clients/BinanceWSClient.hpp)

target_link_libraries(OceanMain PRIVATE
        OceanCore
        ${CURL_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        simdjson::simdjson
)

# ================== TRADING SYSTEM EXECUTABLE ==================
add_executable(TradingSystem ${OCEAN_SRC_DIR}/main.cpp
        src/Clients/BinanceWSClient.cpp
        include/Clients/BinanceWSClient.hpp)

target_include_directories(TradingSystem PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${OCEAN_INCLUDE_DIR}
)

target_link_libraries(TradingSystem PRIVATE
        OceanCore
        OpenSSL::SSL
        OpenSSL::Crypto
)

# ================== RPATH SETUP FOR EXECUTABLES ==================
foreach(target OceanMain TradingSystem)
    set_target_properties(${target} PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "${GCC14_LIB_PATH}"
    )
endforeach()

# ================== INSTALL ==================
install(TARGETS OceanMain DESTINATION bin)
install(TARGETS OceanCore ARCHIVE DESTINATION lib)