#include "Core/MarketData.hpp"
#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

MarketData::MarketData(std::string symbol)
    : symbol_(std::move(symbol)),
      ssl_ctx_(ssl::context::tlsv12_client),
      ioc_(1) {
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(ssl::verify_peer);
    buffer_.reserve(1024);
}

bool MarketData::start() noexcept {
    if (running_.exchange(true)) {
        return false;
    }

    try {
        binance_client_ = std::make_unique<BinanceWSClient>(ioc_, ssl_ctx_, symbol_);
        binance_client_->start();

        // IO context thread
        io_thread_ = std::jthread([this](std::stop_token st) {
            while (!st.stop_requested()) {
                try {
                    ioc_.run();
                    break;
                } catch (const std::exception& e) {
                    std::cerr << "IO context error: " << e.what() << "\n";
                    if (!st.stop_requested()) {
                        std::this_thread::sleep_for(1s);
                        ioc_.restart();
                    }
                }
            }
        });

        // Data processing thread
        processing_thread_ = std::jthread([this](std::stop_token st) {
            while (!st.stop_requested()) {
                if (!binance_client_) continue;

                auto& ws_data = binance_client_->get_market_data();
                std::unique_lock lock(ws_data.mutex, std::try_to_lock);

                if (lock.owns_lock() && ws_data.connected) {
                    std::vector<OrderBook::Order> new_orders;
                    new_orders.push_back({
                        .price = static_cast<float>(ws_data.bid),
                        .amount = 1.0f,
                        .is_bid = true
                    });
                    new_orders.push_back({
                        .price = static_cast<float>(ws_data.ask),
                        .amount = 1.0f,
                        .is_bid = false
                    });

                    {
                        std::lock_guard buffer_lock(buffer_mutex_);
                        buffer_ = std::move(new_orders);
                        buffer_size_.store(buffer_.size());
                    }
                }
                std::this_thread::sleep_for(10ms);
            }
        });

        return true;
    } catch (...) {
        running_ = false;
        return false;
    }
}

void MarketData::stop() noexcept {
    if (!running_) return;

    running_ = false;

    if (binance_client_) {
        binance_client_->stop();
    }

    ioc_.stop();

    if (io_thread_.joinable()) {
        io_thread_.request_stop();
        io_thread_.join();
    }

    if (processing_thread_.joinable()) {
        processing_thread_.request_stop();
        processing_thread_.join();
    }

    binance_client_.reset();
}

std::span<const OrderBook::Order> MarketData::get_updates() noexcept {
    std::lock_guard lock(buffer_mutex_);
    return {buffer_.data(), buffer_size_.load()};
}

MarketData::~MarketData() {
    // Already implemented in your code but needs to be in .cpp file
    stop();
}