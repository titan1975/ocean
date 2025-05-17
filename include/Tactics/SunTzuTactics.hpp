#pragma once
#include "Core/OrderBook.hpp"
#include <immintrin.h>
#include <span>
#include <vector>

// Forward declarations
struct Trade {
    float price;
    float amount;
};

namespace SunTzu {
    enum class MarketPhase { RANGING, TRENDING, CHAOS };

    // Liquidity Raid Configuration
    struct LiquidityRaidConfig {
        float volume_spike_multiplier;
        float time_window_seconds;
        float min_volume_threshold;
    };

    // Deception Tactics
    bool isWeakPoint(const OrderBook& book, float liquidityThreshold);
    float detectFakeout(std::span<const Trade> trades, float squeezeThreshold);
    float stealthEntryPrice(const OrderBook& book, bool is_bid) noexcept;

    // Strategic Adaptation
    void adjustForMarketPhase(MarketPhase phase);
    MarketPhase detectMarketPhase(const std::vector<float>& prices);

    // Liquidity Analysis
    LiquidityRaidConfig detectLiquidityRaid(const OrderBook& book, std::span<const Trade> trades);
}