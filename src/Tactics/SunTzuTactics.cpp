//
// Created by hera on 5/17/25.
//

#include "Tactics/SunTzuTactics.hpp"
#include "Risk/RiskManager.hpp"

bool SunTzu::isWeakPoint(const OrderBook& book, float threshold) {
    // Use corrected method names
    float bidVol = book.total_bid_volume();
    float askVol = book.total_ask_volume();
    return (bidVol < threshold) || (askVol < threshold);
}

void SunTzu::adjustForMarketPhase(MarketPhase phase) {
    switch (phase) {
        case MarketPhase::CHAOS:    RiskManager::adjustForVolatility(4.0); break;
        case MarketPhase::TRENDING: RiskManager::adjustForVolatility(1.0); break;
        case MarketPhase::RANGING:  RiskManager::adjustForVolatility(2.0); break;
    }
}