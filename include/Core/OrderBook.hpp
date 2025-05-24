#pragma once

#include <stdfloat>        // C++23 fixed-width floats (requires C++23 compiler)
#include <vector>
#include <map>
#include <shared_mutex>
#include <immintrin.h> // For __m256, etc., if you plan to use AVX intrinsics for optimization.
                       // Make sure your compiler supports AVX and you enable it.

class OrderBook {
public:
    struct Order {
        std::float32_t price;
        std::float32_t amount;
        bool is_bid; // true for bid, false for ask

        // --- NEW: Constructor to handle double inputs from parsing (e.g., std::stod) ---
        // This constructor explicitly converts double to std::float32_t for price and amount.
        Order(double p, double a, bool bid)
            : price(static_cast<std::float32_t>(p)),
              amount(static_cast<std::float32_t>(a)),
              is_bid(bid) {}

        // --- NEW: Default constructor ---
        // Good practice to have if you might create Order objects without initial values.
        Order() : price(static_cast<std::float32_t>(0.0)),
                  amount(static_cast<std::float32_t>(0.0)),
                  is_bid(false) {}
    };

    // Initialize order book with snapshot data
    void initialize(const std::vector<Order>& snapshot) noexcept;

    // Update the order book with new orders (deltas or full updates for specific levels)
    void update(const std::vector<Order>& orders) noexcept;

    // New methods to get volumes
    // --- FIXED: Changed return types from float to std::float32_t for consistency ---
    [[nodiscard]] std::float32_t total_bid_volume() const noexcept;
    [[nodiscard]] std::float32_t total_ask_volume() const noexcept;
    [[nodiscard]] std::float32_t get_mid_price() const noexcept;
    [[nodiscard]] std::pair<std::float32_t, std::float32_t> get_bbo() const noexcept; // Best Bid Offer (highest bid, lowest ask)

private:
    struct PriceLevel {
        std::float32_t total_amount{0};
        int order_count{0};
    };

    // Using std::map for order books:
    // Bids are typically sorted descending by price (highest bid first), so std::map<price, ...>
    // Asks are typically sorted ascending by price (lowest ask first), so std::map<price, ...>
    // std::map automatically sorts by key. For bids, we'll iterate in reverse to get highest.
    using BookSide = std::map<std::float32_t, PriceLevel>;

    // alignas(64) is for cache line alignment, which can be useful for performance
    // with certain types of access patterns, especially with SIMD instructions.
    // Ensure your compiler supports C++11 alignas.
    alignas(64) BookSide bids_;
    alignas(64) BookSide asks_;

    // std::shared_mutex allows multiple readers or a single writer.
    mutable std::shared_mutex mtx_;
};