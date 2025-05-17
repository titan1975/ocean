#include "Tactics/SunTzuTactics.hpp"
#include "Risk/RiskManager.hpp"

bool SunTzu::isWeakPoint(const OrderBook& book, float threshold) {
    float bidVol = book.total_bid_volume();
    float askVol = book.total_ask_volume();
    return (bidVol < threshold) || (askVol < threshold);
}

float SunTzu::stealthEntryPrice(const OrderBook& book, bool is_bid) noexcept {
    auto [best_bid, best_ask] = book.get_bbo();
    return is_bid ? best_bid * 0.998f : best_ask * 1.002f;
}

void SunTzu::adjustForMarketPhase(MarketPhase phase) {
    switch (phase) {
        case MarketPhase::CHAOS:    RiskManager::adjustForVolatility(4.0f); break;
        case MarketPhase::TRENDING: RiskManager::adjustForVolatility(1.0f); break;
        case MarketPhase::RANGING:  RiskManager::adjustForVolatility(2.0f); break;
    }
}

SunTzu::MarketPhase SunTzu::detectMarketPhase(const std::vector<float>& prices) {
    if (prices.empty()) return MarketPhase::CHAOS;

    // Implementation of phase detection logic
    // ... (your actual implementation here)

    return MarketPhase::RANGING; // default
}

SunTzu::LiquidityRaidConfig SunTzu::detectLiquidityRaid(const OrderBook& book, std::span<const Trade> trades) {
    return {
        .volume_spike_multiplier = 2.5f,
        .time_window_seconds = 30.0f,
        .min_volume_threshold = 10000.0f
    };
}