#include "Risk/RiskManager.hpp"

bool RiskManager::isTradeAllowed(double riskPercent) {
    return riskPercent <= maxSingleTradeRisk();
}

double RiskManager::calculatePositionSize(double entryPrice, double stopLossPrice, double accountBalance) {
    double riskPerUnit = entryPrice - stopLossPrice;
    return (riskPerUnit <= 0.0) ? 0.0 : std::floor((accountBalance * currentMaxRisk) / riskPerUnit);
}

void RiskManager::adjustForVolatility(double multiplier) {
    currentMaxRisk = std::max(0.002, maxSingleTradeRisk() / multiplier);
}

bool RiskManager::shouldStopTrading(double dailyLossPercent) {
    return dailyLossPercent >= maxDailyLoss();
}

double RiskManager::getAccountBalance() {
    return accountBalance;
}

void RiskManager::setAccountBalance(double newBalance) {
    accountBalance = newBalance;
}

double RiskManager::getDailyLossPercent(double initialBalance, double currentBalance) {
    if (initialBalance <= 0) return 0.0;
    return (initialBalance - currentBalance) / initialBalance;
}