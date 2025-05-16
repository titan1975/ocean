//
// Created by hera on 5/17/25.
//
#include "SunTzuTactics.hpp"
#include "RiskManager.hpp"

bool SunTzu::isWeakPoint(const OrderBook& book, float threshold) {
    __m256 thresh = _mm256_set1_ps(threshold);
    __m256 vols = _mm256_set_ps(0, 0, 0, 0, 0, 0, book.getBidVolume(), book.getAskVolume());
    return _mm256_movemask_ps(_mm256_cmp_ps(vols, thresh, _CMP_LT_OQ)) != 0;
}

void SunTzu::adjustForMarketPhase(MarketPhase phase) {
    switch (phase) {
        case MarketPhase::CHAOS:    RiskManager::adjustForVolatility(4.0); break;
        case MarketPhase::TRENDING: RiskManager::adjustForVolatility(1.0); break;
        case MarketPhase::RANGING:  RiskManager::adjustForVolatility(2.0); break;
    }
}