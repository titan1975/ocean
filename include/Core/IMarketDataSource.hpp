#pragma once
#include <span>
#include <atomic>
#include "OrderBook.hpp"

class IMarketDataSource {
public:
    virtual std::span<const OrderBook::Order> get_updates() noexcept = 0;
    virtual bool start() noexcept = 0;
    virtual void stop() noexcept = 0;
    virtual ~IMarketDataSource() = default;
protected:
    IMarketDataSource() = default;
};