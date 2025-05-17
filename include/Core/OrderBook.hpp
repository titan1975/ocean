#pragma once
#include <stdfloat>       // C++23 fixed-width floats
#include <vector>
#include <map>
#include <shared_mutex>
#include <immintrin.h>

class OrderBook {
public:
    struct Order {
        std::float32_t price;
        std::float32_t amount;
        bool is_bid;
    };

    // New methods to get volumes
    [[nodiscard]] float total_bid_volume() const noexcept;
    [[nodiscard]] float total_ask_volume() const noexcept;
    [[nodiscard]]  float get_mid_price() const noexcept;
    void update(const std::vector<Order>& orders) noexcept;
    [[nodiscard]] std::pair<float, float> get_bbo() const noexcept;

private:
    struct PriceLevel {
        std::float32_t total_amount{0};
        int order_count{0};
    };

    using BookSide = std::map<std::float32_t, PriceLevel>;
    alignas(64) BookSide bids_, asks_;
    mutable std::shared_mutex mtx_;
};