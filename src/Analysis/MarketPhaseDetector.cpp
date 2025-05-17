#include "Analysis/MarketPhaseDetector.hpp"
#include <numeric>
#include <cmath>
#include <algorithm> // For std::max/min

void MarketPhaseDetector::update(float price) {
    prices_.push_back(price);
    if (prices_.size() > 100) {
        prices_.erase(prices_.begin());
    }
}

SunTzu::MarketPhase MarketPhaseDetector::getPhase() const {
    if (isChaos()) return SunTzu::MarketPhase::CHAOS;
    if (isTrending()) return SunTzu::MarketPhase::TRENDING;
    return SunTzu::MarketPhase::RANGING;
}

//--------------------------------------------------------------------
// TRENDING DETECTION: "Is the market making sustained directional moves?"
//--------------------------------------------------------------------
bool MarketPhaseDetector::isTrending() const {
    if (prices_.size() < 20) return false; // Not enough data

    // Calculate Average Directional Movement (ADX-like logic)
    float avgUpMove = 0.0f, avgDownMove = 0.0f;
    for (size_t i = 1; i < prices_.size(); ++i) {
        const float change = prices_[i] - prices_[i-1];
        avgUpMove += std::max(change, 0.0f);    // Upward movement
        avgDownMove += std::abs(std::min(change, 0.0f)); // Downward movement
    }
    avgUpMove /= prices_.size();
    avgDownMove /= prices_.size();

    // Trending if one side dominates by 30%
    const float strength = std::abs(avgUpMove - avgDownMove) /
                         (avgUpMove + avgDownMove + 1e-5f); // Avoid division by zero
    return strength > 0.3f; // 30% strength threshold
}

//--------------------------------------------------------------------
// CHAOS DETECTION: "Is the market erratic and volatile?"
//--------------------------------------------------------------------
bool MarketPhaseDetector::isChaos() const {
    if (prices_.size() < 10) return false;

    // Calculate average true range (ATR-like)
    float avgRange = 0.0f;
    for (size_t i = 1; i < prices_.size(); ++i) {
        avgRange += std::abs(prices_[i] - prices_[i-1]);
    }
    avgRange /= prices_.size();

    // Chaos = volatility spikes (2x average)
    return avgRange > 2.0f * getVolatility();
}

//--------------------------------------------------------------------
// VOLATILITY CALCULATION: Standard deviation of prices
//--------------------------------------------------------------------
float MarketPhaseDetector::getVolatility() const {
    if (prices_.empty()) return 0.0f;

    // Calculate mean
    const float mean = std::accumulate(prices_.begin(), prices_.end(), 0.0f)
                     / prices_.size();

    // Calculate variance
    float variance = 0.0f;
    for (float price : prices_) {
        variance += std::pow(price - mean, 2);
    }
    variance /= prices_.size();

    return std::sqrt(variance); // Standard deviation
}