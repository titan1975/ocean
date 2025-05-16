#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include "Core/MarketData.hpp"
#include "Core/OrderBook.hpp"
#include "Strategy/LiquidityRaidDetector.hpp"

// Global kill switch for graceful shutdown
std::atomic<bool> global_blood_moon{false};

//------------------------------------------------------------------
// L I Q U I D   B L O O D   S T R A T E G Y
//------------------------------------------------------------------
void liquid_blood(MarketData& market, OrderBook& book) {
    LiquidityRaidDetector raid_detector({
        .min_wick_ratio = 1.8f,
        .volume_spike_multiplier = 2.5f
    });

    while (!global_blood_moon) {
        // Get market updates (zero-copy span)
        auto updates = market.get_updates();
        if (updates.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // Update order book
          book.update(std::vector<OrderBook::Order>(updates.begin(), updates.end()));

        // Detect liquidity raids
        const auto [best_bid, best_ask] = book.get_bbo();
        const float mid_price = (best_bid + best_ask) * 0.5f;

        if (raid_detector.detect_raid(updates, mid_price)) {
            std::cout << "⚡ LIQUIDITY RAID DETECTED! PRICE: " << mid_price << "\n";
            // TODO: Trigger orders or alerts
        }
    }
    std::cout << "💀 Liquid Blood strategy terminated.\n";
}

//------------------------------------------------------------------
// M A I N   W A R   M A C H I N E
//------------------------------------------------------------------
int main() {
    try {
        // Initialize core components
        MarketData market("127.0.0.1", 1337);  // Replace with real endpoint
        OrderBook book;

        if (!market.start()) {
            throw std::runtime_error("Failed to start market data feed");
        }

        // Strategy threads
        std::vector<std::jthread> strategy_warriors;

        // Spawn Liquid Blood in its own thread
        strategy_warriors.emplace_back([&] {
            liquid_blood(market, book);
        });

        std::cout << "🔥 Trading system online. Press Ctrl+C to terminate.\n";

        // Wait for termination signal
        std::signal(SIGINT, [](int) {
            global_blood_moon = true;
        });

        while (!global_blood_moon) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Cleanup
        market.stop();
        strategy_warriors.clear();  // Joins all threads

    } catch (const std::exception& e) {
        std::cerr << "💥 MAIN CRASH: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}