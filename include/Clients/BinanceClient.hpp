#pragma once
#include "Core/IMarketDataSource.hpp"
#include <boost/lockfree/spsc_queue.hpp>

#include "Core/OrderBook.hpp"

class BinanceClient final : public IMarketDataSource {
public:
     explicit BinanceClient(std::string symbol);
    std::span<const OrderBook::Order> get_updates() noexcept override;
    bool start() noexcept override;
    void stop() noexcept override;
private:
    std::string symbol_;
    boost::lockfree::spsc_queue<OrderBook::Order> buffer_{1024};
    std::atomic<bool> running_{false};
};
