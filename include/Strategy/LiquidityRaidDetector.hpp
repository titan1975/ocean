#pragma once
#include "Core/OrderBook.hpp"
#include <span>

class LiquidityRaidDetector {  // Changed from LiquidityDetector
public:
    struct Config {
        float min_wick_ratio = 1.8f;
        float volume_spike_multiplier = 2.5f;
    };

    explicit LiquidityRaidDetector(Config cfg) : cfg_(cfg) {}

    [[nodiscard]] bool detect_raid(
        std::span<const OrderBook::Order> trades,
        float current_price
    ) const noexcept;

private:
    Config cfg_;
};