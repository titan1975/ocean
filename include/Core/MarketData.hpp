#pragma once
#include "Core/OrderBook.hpp"
#include "Clients/BinanceWSClient.hpp"
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>

class MarketData {
public:
    MarketData(std::string symbol = "btcusdt");

     ~MarketData();

    bool start() noexcept;
    void stop() noexcept;
   // void initialize(const std::vector<Order>& snapshot);

    std::span<const OrderBook::Order> get_updates() noexcept;

    std::vector<OrderBook::Order> get_snapshot();

private:
    void process_binance_data(const BinanceWSClient::MarketData& ws_data);

    std::string symbol_;
    std::atomic<bool> running_{false};
    std::jthread io_thread_;
    std::jthread processing_thread_;
    std::vector<OrderBook::Order> buffer_;
    std::mutex buffer_mutex_;

    std::atomic<size_t> buffer_size_{0};

    // Binance WebSocket client
    std::unique_ptr<BinanceWSClient> binance_client_;
    net::io_context ioc_;
    ssl::context ssl_ctx_{ssl::context::tlsv12_client};
};