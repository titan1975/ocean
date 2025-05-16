#include "MarketPhaseDetector.hpp"

SunTzu::MarketPhase MarketPhaseDetector::getPhase() const {
    if (isChaos()) return SunTzu::MarketPhase::CHAOS;
    if (isTrending()) return SunTzu::MarketPhase::TRENDING;
    return SunTzu::MarketPhase::RANGING;
}