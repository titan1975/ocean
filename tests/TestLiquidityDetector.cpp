#include "Strategy/LiquidityDetector.hpp"
#include <gtest/gtest.h>

#include "Core/OrderBook.hpp"
#include "Strategy/LiquidityRaidDetector.hpp"

TEST(LiquidityDetectorTest, DetectsVolumeSpike) {
    LiquidityDetector::Config cfg;
    LiquidityDetector detector(cfg);

    std::vector<OrderBook::Order> trades = {
        {100.0f, 10.0f, true},
        {101.0f, 10.0f, true},
        {102.0f, 10.0f, true},
        {103.0f, 50.0f, true},  // Volume spike
        {104.0f, 10.0f, true}
    };

    EXPECT_TRUE(detector.detect_raid(trades, 103.5f));
}