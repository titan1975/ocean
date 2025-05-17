#pragma once
#include "Core/OrderBook.hpp"
#include <span>

struct LiquidityRaidConfig {
    float volume_spike_multiplier;
    float min_wick_ratio;
};

class LiquidityRaidDetector {
public:
    explicit LiquidityRaidDetector(LiquidityRaidConfig cfg) : cfg_(cfg) {}

    bool detect_raid(std::span<const OrderBook::Order> orders, float threshold) const noexcept;
    bool detect_volume_spike_and_wick(std::span<const OrderBook::Order> trades, float current_price) const;

private:
    LiquidityRaidConfig cfg_;
};