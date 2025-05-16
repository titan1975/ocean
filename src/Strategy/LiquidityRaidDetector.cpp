#include "Strategy/LiquidityDetector.hpp"
#include "Core/OrderBook.hpp"
#include <algorithm>  // Required for std::minmax_element
#include <numeric>    // Required for std::accumulate

bool LiquidityDetector::detect_raid(
    std::span<const OrderBook::Order> trades,
    float current_price
) const noexcept {
    if (trades.empty()) return false;

    // Volume spike check (using SIMD-friendly accumulation)
    const float total_volume = std::accumulate(
        trades.begin(), trades.end(), 0.0f,
        [](float sum, const OrderBook::Order& trade) {
            return sum + trade.amount;
        }
    );
    const float avg_volume = total_volume / trades.size();
    const bool volume_spike = trades.back().amount > (avg_volume * cfg_.volume_spike_multiplier);

    // Wick ratio calculation
    const auto [min_it, max_it] = std::minmax_element(
        trades.begin(), trades.end(),
        [](const OrderBook::Order& a, const OrderBook::Order& b) {
            return a.price < b.price;
        }
    );

    const float price_range = max_it->price - min_it->price;
    if (price_range <= std::numeric_limits<float>::epsilon()) return false;  // Avoid division by zero

    const float wick_ratio = (current_price - min_it->price) / price_range;
    const bool wick_condition = wick_ratio > cfg_.min_wick_ratio;

    return volume_spike && wick_condition;
}