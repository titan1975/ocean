#pragma once
#include <cmath>

class RiskManager {
public:
    static bool isTradeAllowed(double riskPercent);
    static double calculatePositionSize(double entryPrice, double stopLossPrice, double accountBalance);
    static void adjustForVolatility(double multiplier);
    static bool shouldStopTrading(double dailyLossPercent);

    // Account balance methods
    static double getAccountBalance();
    static void setAccountBalance(double newBalance);

    // Add this function to calculate daily loss percentage
    static double getDailyLossPercent(double initialBalance, double currentBalance);
private:
    static constexpr double maxSingleTradeRisk() { return 0.01; }
    static constexpr double maxDailyLoss() { return 0.03; }
    static inline double currentMaxRisk = maxSingleTradeRisk();
    static inline double accountBalance = 0.0;  // Initialize to 0 or some default
};