//
// Created by hera on 5/17/25.
//
#include "Risk/RiskManager.hpp"


// Explicit instantiation of critical paths
template double RiskManager::calculatePositionSize(double, double, double);
template void RiskManager::adjustForVolatility(double);