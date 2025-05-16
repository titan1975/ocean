#include "Core/MarketData.hpp"
#include "Core/OrderBook.hpp"
#include <cstring>
#include <iostream>
#include <zlib.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <unistd.h>

MarketData::MarketData(std::string_view endpoint, uint16_t port)
    : endpoint_(endpoint), port_(port) {
    buffer_.reserve(1024);
    if (endpoint.empty()) {
        throw std::invalid_argument("Endpoint cannot be empty");
    }
}

MarketData::~MarketData() {
    stop();
}

bool MarketData::start() noexcept {
    if (running_.exchange(true)) {
        return false;  // Already running
    }
    try {
        io_thread_ = std::jthread([this](std::stop_token st) {
            while (!st.stop_requested() && running_) {
                io_thread();
            }
        });
        return true;
    } catch (...) {
        running_ = false;
        return false;
    }
}

void MarketData::stop() noexcept {
    running_ = false;
    if (io_thread_.joinable()) {
        io_thread_.request_stop();
        io_thread_.join();
    }
    if (int fd = fd_.load(); fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        fd_.store(-1);
    }
    connected_.store(false);
}

bool MarketData::try_connect() noexcept {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, endpoint_.c_str(), &addr.sin_addr) <= 0) {
        close(fd);
        return false;
    }

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        if (errno != EINPROGRESS) {
            close(fd);
            return false;
        }

        // Wait for connection completion
        pollfd pfd{fd, POLLOUT, 0};
        if (poll(&pfd, 1, 1000) <= 0) {  // 1s timeout
            close(fd);
            return false;
        }

        int err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err) {
            close(fd);
            return false;
        }
    }

    fd_.store(fd);
    connected_.store(true);
    reconnect_attempts_.store(0);
    return true;
}

void MarketData::io_thread() noexcept {
    if (!connected_.load() && !try_connect()) {
        apply_backoff();
        return;
    }

    const int fd = fd_.load();
    if (fd < 0) {
        connected_.store(false);
        return;
    }

    std::array<std::byte, 8192> recv_buf;
    ssize_t n = recv(fd, recv_buf.data(), recv_buf.size(), MSG_DONTWAIT);
    if (n <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        connected_.store(false);
        close(fd);
        fd_.store(-1);
        apply_backoff();
        return;
    }

    // Full message validation
    if (n < static_cast<ssize_t>(sizeof(BinMessage))) {
        std::cerr << "Invalid message: too short\n";
        return;
    }

    const auto* msg = reinterpret_cast<const BinMessage*>(recv_buf.data());
    if (msg->magic != 0xDEADBEEF) {
        std::cerr << "Invalid message: bad magic\n";
        return;
    }

    const uint32_t expected_crc = msg->crc32;
    const uint32_t actual_crc = calculate_crc32(
        reinterpret_cast<const std::byte*>(msg) + sizeof(msg->crc32),
        sizeof(BinMessage) - sizeof(msg->crc32)
    );
    if (expected_crc != actual_crc) {
        std::cerr << "CRC32 mismatch\n";
        return;
    }

    const size_t expected_size = sizeof(BinMessage) + msg->count * sizeof(BinOrder);
    if (n < static_cast<ssize_t>(expected_size)) {
        std::cerr << "Incomplete message\n";
        return;
    }

    // Process orders
    std::vector<OrderBook::Order> new_orders;
    new_orders.reserve(msg->count);

    const auto* orders = reinterpret_cast<const BinOrder*>(msg + 1);
    for (uint16_t i = 0; i < msg->count; ++i) {
        new_orders.push_back({
            .price = orders[i].price,
            .amount = orders[i].amount,
            .is_bid = (orders[i].side == 0)
        });
    }

    // Atomic update
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        buffer_ = std::move(new_orders);
        buffer_size_.store(buffer_.size());
    }
}

uint32_t MarketData::calculate_crc32(const void* data, size_t length) const noexcept {
    return crc32(0L, reinterpret_cast<const Bytef*>(data), length);
}

void MarketData::apply_backoff() noexcept {
    const uint32_t attempts = reconnect_attempts_.fetch_add(1);
    const uint32_t max_delay_ms = 5000;
    const uint32_t base_delay_ms = 100;

    std::uniform_int_distribution<> jitter(-100, 100);
    const uint32_t delay_ms = std::min(
        base_delay_ms * (1 << std::min(attempts, 10U)) + jitter(gen_),
        max_delay_ms
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
}

std::span<const OrderBook::Order> MarketData::get_updates() noexcept {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return {buffer_.data(), buffer_size_.load()};
}