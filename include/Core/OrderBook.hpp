#pragma once
#include <stdfloat>       // C++23 fixed-width floats
#include <vector>
#include <map>            // For price level merging
#include <shared_mutex>   // Reader-writer lock
#include <immintrin.h>    // AVX2

class OrderBook {
public:
    struct Order {
        std::float32_t price;
        std::float32_t amount;
        bool is_bid;
    };

    // Update order book with new orders (thread-safe)
    void update(const std::vector<Order>& orders) noexcept;

    // Get best bid/offer (thread-safe)
    [[nodiscard]] std::pair<float, float> get_bbo() const noexcept;

private:
    // Price level aggregation
    struct PriceLevel {
        std::float32_t total_amount{0};
        int order_count{0};
    };

    using BookSide = std::map<std::float32_t, PriceLevel>;  // Sorted by price

    // Data members
    alignas(64) BookSide bids_, asks_;  // Cache-line aligned
    mutable std::shared_mutex mtx_;     // Replaces spinlock
};