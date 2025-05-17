
#pragma once
#include "Tactics/SunTzuTactics.hpp"
#include <vector>

class MarketPhaseDetector {
public:
    void update(float price);
    SunTzu::MarketPhase getPhase() const;

private:
    std::vector<float> prices_;
    bool isTrending() const;
    bool isChaos() const;
    float getVolatility() const;
};






// #pragma once
// #include <vector>
// #include <cmath>
//
// enum class MarketPhase {
//     RANGING,    // Sideways chop (boring)
//     TRENDING,   // Strong directional move
//     CHAOS        // High volatility
// };
//
// class MarketPhaseDetector {
// public:
//     // Update with latest price (call every tick/candle)
//     void update(float price) {
//         prices.push_back(price);
//         if (prices.size() > 100) prices.erase(prices.begin());
//     }
//
//     // Sun Tzu Rule: "Know your terrain"
//     MarketPhase getPhase() const {
//         if (isChaos()) return MarketPhase::CHAOS;
//         if (isTrending()) return MarketPhase::TRENDING;
//         return MarketPhase::RANGING;
//     }
//
// private:
//     std::vector<float> prices;
//
//     bool isTrending() const {
//         if (prices.size() < 20) return false;
//
//         // Simple ADX-like logic
//         float avgUpMove = 0.0f, avgDownMove = 0.0f;
//         for (size_t i = 1; i < prices.size(); ++i) {
//             float change = prices[i] - prices[i-1];
//             if (change > 0) avgUpMove += change;
//             else avgDownMove += std::abs(change);
//         }
//         float strength = std::abs(avgUpMove - avgDownMove) / (avgUpMove + avgDownMove);
//         return strength > 0.3f; // 30% trend strength threshold
//     }
//
//     bool isChaos() const {
//         if (prices.size() < 10) return false;
//
//         // Check for erratic wicks/spikes
//         float avgRange = 0.0f;
//         for (size_t i = 1; i < prices.size(); ++i) {
//             avgRange += std::abs(prices[i] - prices[i-1]);
//         }
//         avgRange /= prices.size();
//         return (avgRange > (2.0f * getAverageVolatility()));
//     }
//
//     float getAverageVolatility() const { /* ... */ }
// };