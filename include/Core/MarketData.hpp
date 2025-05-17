#pragma once
#include <vector>
#include <atomic>
#include <span>
#include <string_view>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <immintrin.h>
#include <chrono>
#include <random>
#include <array>

#include "OrderBook.hpp"

class OrderBook; // Forward declaration

class MarketData {
public:

    // Add forward declaration
    struct BinMessage;
    struct BinOrder;

    #pragma pack(push, 1)
    struct BinMessage {
        uint32_t magic = 0xDEADBEEF;  // Network byte order
        uint32_t crc32;
        uint64_t timestamp;  // nanos
        uint16_t count;      // orders in packet
    };
    struct BinOrder {
        float price;
        float amount;
        uint8_t side;  // 0=bid, 1=ask
    };
#pragma pack(pop)

    explicit MarketData(std::string_view endpoint, uint16_t port = 443);
    ~MarketData();

    // Non-copyable
    MarketData(const MarketData&) = delete;
    MarketData& operator=(const MarketData&) = delete;

    // Thread-safe interface
    bool start() noexcept;
    void stop() noexcept;
    std::span<const OrderBook::Order> get_updates() noexcept;

private:
    void io_thread() noexcept;
    bool try_connect() noexcept;
    uint32_t calculate_crc32(const void* data, size_t length) const noexcept;
    void apply_backoff() noexcept;

    // Connection state
    std::atomic<int> fd_{-1};
    std::string endpoint_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::atomic<uint16_t> port_;
    std::jthread io_thread_;

    // Data buffer
    std::vector<OrderBook::Order> buffer_;
    std::atomic<size_t> buffer_size_{0};
    std::mutex buffer_mutex_;  // Protects buffer_ and buffer_size_

    // Backoff state
    std::atomic<uint32_t> reconnect_attempts_{0};
    std::random_device rd_;
    std::mt19937 gen_{rd_()};

    // Constants
    static constexpr size_t kRecvBufferSize = 8192;
    static constexpr uint32_t kMaxBackoffMs = 5000;
    static constexpr uint32_t kBaseBackoffMs = 100;
};