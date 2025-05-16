#pragma once
#include <vector>
#include "SunTzuTactics.hpp"

class MarketPhaseDetector {
public:
    void update(float price) {
        prices.push_back(price);
        if (prices.size() > 100) prices.erase(prices.begin());
    }

    SunTzu::MarketPhase getPhase() const;

private:
    std::vector<float> prices;
    bool isTrending() const;
    bool isChaos() const;
};