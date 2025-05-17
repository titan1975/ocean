
#include "Core/OrderBook.hpp"

#include <algorithm>
#include <mutex>

void OrderBook::update(const std::vector<Order>& orders) noexcept {
    // Writer lock (exclusive access)
    std::unique_lock lock(mtx_);

    for (const auto& order : orders) {
        auto& side = order.is_bid ? bids_ : asks_;
        auto [it, inserted] = side.try_emplace(order.price, PriceLevel{});

        // Aggregate amounts at same price level
        it->second.total_amount += order.amount;
        it->second.order_count++;
    }

    // Prune empty levels (optional)
    for (auto* side : {&bids_, &asks_}) {
        for (auto it = side->begin(); it != side->end(); ) {
            if (it->second.total_amount <= 0.0f) {
                it = side->erase(it);
            } else {
                ++it;
            }
        }
    }
}

// In OrderBook.cpp
float OrderBook::get_mid_price() const noexcept {
    std::shared_lock lock(mtx_); // Shared lock for thread safety

    if (bids_.empty() || asks_.empty()) {
        return 0.0f; // Or handle this case appropriately for your application
    }

    float best_bid = bids_.rbegin()->first;
    float best_ask = asks_.begin()->first;

    return (best_bid + best_ask) / 2.0f;
}

std::pair<float, float> OrderBook::get_bbo() const noexcept {
    // Reader lock (shared access)
    std::shared_lock lock(mtx_);

    const float best_bid = bids_.empty() ? 0.0f : bids_.rbegin()->first;  // Highest bid
    const float best_ask = asks_.empty() ? 0.0f : asks_.begin()->first;   // Lowest ask

    return {best_bid, best_ask};
}

// In OrderBook.cpp
float OrderBook::total_bid_volume() const noexcept {
    std::shared_lock lock(mtx_);
    float total = 0.0f;
    for (const auto& [price, level] : bids_) {
        total += level.total_amount;
    }
    return total;
}

float OrderBook::total_ask_volume() const noexcept {
    std::shared_lock lock(mtx_);
    float total = 0.0f;
    for (const auto& [price, level] : asks_) {
        total += level.total_amount;
    }
    return total;
}