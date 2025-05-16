#include "Strategy/LiquidityDetector.hpp"
#include <algorithm>  // Required for std::minmax_element
#include <numeric>    // Required for std::accumulate

bool LiquidityDetector::detect_raid(
    std::span<const OrderBook::Order> trades,
    float current_price
) const noexcept {
    if (trades.size() < 5) return false;

    // Volume spike check
    const float avg_volume = std::accumulate(
        trades.begin(), trades.end(), 0.0f,
        [](float sum, const auto& trade) {
            return sum + trade.amount;
        }
    ) / trades.size();

    const float last_volume = trades.back().amount;
    const bool volume_ok = last_volume > (avg_volume * cfg_.volume_spike_multiplier);

    // Wick ratio calculation
    const auto [min, max] = std::minmax_element(
        trades.begin(), trades.end(),
        [](const auto& a, const auto& b) { return a.price < b.price; }
    );

    const float wick_ratio = (current_price - min->price) / (max->price - min->price);
    const bool wick_ok = wick_ratio > cfg_.min_wick_ratio;

    return volume_ok && wick_ok;
}