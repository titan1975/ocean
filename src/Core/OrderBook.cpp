
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

std::pair<float, float> OrderBook::get_bbo() const noexcept {
    // Reader lock (shared access)
    std::shared_lock lock(mtx_);

    const float best_bid = bids_.empty() ? 0.0f : bids_.rbegin()->first;  // Highest bid
    const float best_ask = asks_.empty() ? 0.0f : asks_.begin()->first;   // Lowest ask

    return {best_bid, best_ask};
}