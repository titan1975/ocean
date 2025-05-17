#include "Strategy/LiquidityRaidDetector.hpp"
#include <algorithm>
#include <limits>

bool LiquidityRaidDetector::detect_raid(std::span<const OrderBook::Order> orders, float threshold) const noexcept {
    if (orders.empty()) return false;

    float total_amount = 0.0f;
    for (const auto& order : orders) {
        total_amount += order.amount;
    }

    return total_amount > threshold;
}

bool LiquidityRaidDetector::detect_volume_spike_and_wick(
    std::span<const OrderBook::Order> trades,
    float current_price) const
{
    if (trades.empty()) return false;

    // Calculate total volume
    float total_volume = 0.0f;
    for (const auto& trade : trades) {
        total_volume += trade.amount;
    }

    // Check for volume spike
    const float avg_volume = total_volume / trades.size();
    const bool volume_spike = trades.back().amount > (avg_volume * cfg_.volume_spike_multiplier);

    // Find price extremes
    const auto [min_it, max_it] = std::minmax_element(
        trades.begin(), trades.end(),
        [](const OrderBook::Order& a, const OrderBook::Order& b) {
            return a.price < b.price;
        }
    );

    // Calculate wick ratio
    const float price_range = max_it->price - min_it->price;
    if (price_range <= std::numeric_limits<float>::epsilon()) {
        return false;  // Avoid division by zero
    }

    const float wick_ratio = (current_price - min_it->price) / price_range;
    const bool wick_condition = wick_ratio > cfg_.min_wick_ratio;

    return volume_spike && wick_condition;
}