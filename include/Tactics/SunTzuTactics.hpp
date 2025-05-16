#pragma once
#include "OrderBook.hpp"
#include <immintrin.h>

namespace SunTzu {
    enum class MarketPhase { RANGING, TRENDING, CHAOS };

    // Deception Tactics
    bool isWeakPoint(const OrderBook& book, float liquidityThreshold);
    float detectFakeout(std::span<const Trade> trades, float squeezeThreshold);
    float stealthEntryPrice(const OrderBook& book, bool isBuy);
    
    // Strategic Adaptation
    void adjustForMarketPhase(MarketPhase phase);
    MarketPhase detectMarketPhase(const std::vector<float>& prices);
}