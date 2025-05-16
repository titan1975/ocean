#pragma once
#include <cmath>

class RiskManager {
public:
    static bool isTradeAllowed(double riskPercent) { 
        return riskPercent <= maxSingleTradeRisk(); 
    }

    static double calculatePositionSize(double entryPrice, 
                                     double stopLossPrice,
                                     double accountBalance) {
        double riskPerUnit = entryPrice - stopLossPrice;
        if (riskPerUnit <= 0.0) return 0.0;
        return std::floor((accountBalance * currentMaxRisk) / riskPerUnit);
    }

    static void adjustForVolatility(double multiplier) {
        currentMaxRisk = std::max(0.002, maxSingleTradeRisk() / multiplier);
    }

    static bool shouldStopTrading(double dailyLossPercent) {
        return dailyLossPercent >= maxDailyLoss();
    }

private:
    static constexpr double maxSingleTradeRisk() { return 0.01; }
    static constexpr double maxDailyLoss() { return 0.03; }
    static inline double currentMaxRisk = maxSingleTradeRisk();
};