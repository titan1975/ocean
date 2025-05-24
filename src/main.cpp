#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>
#include <vector>
#include <memory>
#include <mutex>
#include <iomanip> // For std::setw, std::setprecision
#include <chrono>  // For std::chrono
#include <span>    // For std::span

// Boost.Asio for networking
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

// Your project headers
#include "Strategy/LiquidityRaidDetector.hpp" // Now includes your detect_raid
#include "Risk/RiskManager.hpp"
#include "Tactics/SunTzuTactics.hpp"
#include "Analysis/MarketPhaseDetector.hpp"
#include "Clients/BinanceWSClient.hpp"
#include "Core/MarketData.hpp"      // Contains MarketData and MarketDataSnapshot
#include "Core/OrderBook.hpp"      // Contains OrderBook and OrderBook::Order

// Global kill switch
std::atomic<bool> global_blood_moon{false};
std::mutex cout_mutex; // Mutex for thread-safe cout operations

//------------------------------------------------------------------
// L I Q U I D   B L O O D   S T R A T E G Y  (SUN TZU EDITION)
//------------------------------------------------------------------
// Pass MarketData and OrderBook by REFERENCE, as they are shared, live objects.
void liquid_blood(MarketData& market_data_instance, OrderBook& book, MarketPhaseDetector& phase_detector) {
    std::cout << "🟢 Starting Liquid Blood strategy thread\n";

    LiquidityRaidDetector raid_detector({
        .volume_spike_multiplier = 2.5f,
        .min_wick_ratio = 1.8f
    });
    std::cout << "🔧 Liquidity Raid Detector configured: "
              << "Volume Spike=" << 2.5f << ", Min Wick Ratio=" << 1.8f << "\n";

    while (!global_blood_moon) {
        // Add a small sleep to prevent busy-waiting if the loop runs too fast without updates
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep for 100ms

        std::cout << "\n--- NEW STRATEGY CYCLE ---\n";

        // 1. Get Market Data Snapshot
        std::cout << "🔄 Fetching market updates snapshot...\n";
        MarketDataSnapshot current_market_snapshot = market_data_instance.get_updates();

        // Check if market is connected before proceeding
        if (!current_market_snapshot.connected) {
            std::cout << "⚠️ Market not connected. Waiting...\n";
            continue;
        }

        // 2. Update Market Phase
        float mid_price = book.get_mid_price(); // Get mid-price from the shared OrderBook
        std::cout << "📈 Current mid price (from OrderBook): " << mid_price << "\n";
        std::cout << "🧠 Updating market phase detector...\n";
        phase_detector.update(mid_price);
        auto phase = phase_detector.getPhase();
        std::cout << "🌐 Market Phase: ";
        switch(phase) {
            case SunTzu::MarketPhase::TRENDING: std::cout << "TRENDING"; break;
            case SunTzu::MarketPhase::RANGING: std::cout << "RANGING"; break;
            case SunTzu::MarketPhase::CHAOS: std::cout << "CHAOS"; break;
        }
        std::cout << "\n";

        // 3. Adjust Strategy
        std::cout << "🎯 Adjusting tactics for current phase...\n";
        SunTzu::adjustForMarketPhase(phase); // Assuming static method

        // 4. Check Trading Conditions
        std::cout << "🔍 Checking for weak points in order book...\n";
        if (SunTzu::isWeakPoint(book, 5000.0f)) { // Use the shared OrderBook instance
            std::cout << "🎯 Weak point detected! Preparing trade...\n";

            // 5. Calculate Entry
            float entry = SunTzu::stealthEntryPrice(book, true); // Use the shared OrderBook instance
            float stop_loss = entry * 0.95f;
            std::cout << "💰 Entry Price: " << entry
                      << " | Stop Loss: " << stop_loss << " (" << (entry-stop_loss) << " risk)\n";

            // 6. Risk Management
            double balance = RiskManager::getAccountBalance(); // Assuming static method
            std::cout << "💳 Account Balance: " << balance << "\n";
            double size = RiskManager::calculatePositionSize(entry, stop_loss, balance); // Assuming static method
            std::cout << "📏 Calculated Position Size: " << size << "\n";

            if (RiskManager::isTradeAllowed(0.01)) { // Assuming static method
                std::cout << "✅ Risk check passed (1% risk allowed)\n";

                // --- LIQUIDITY DETECTOR INTEGRATION ---
                // Now we have `current_market_snapshot.recent_depth_updates` which is a std::vector<OrderBook::Order>
                // We can pass a std::span to the detect_raid function.
                if (!current_market_snapshot.recent_depth_updates.empty()) {
                    std::span<const OrderBook::Order> updates_span(current_market_snapshot.recent_depth_updates);
                    if (raid_detector.detect_raid(updates_span, mid_price)) {
                        std::cout << "⚡ RAID DETECTED! EXECUTING TRADE:\n";
                        std::cout << "   ➡️ Entry: " << entry << "\n";
                        std::cout << "   🛑 Stop: " << stop_loss << "\n";
                        std::cout << "   📊 Size: " << size << "\n";
                        // TODO: Actual execution would go here
                    } else {
                        std::cout << "🔍 No liquidity raid detected\n";
                    }
                } else {
                    std::cout << "🔍 No recent depth updates to check for raid.\n";
                }

            } else {
                std::cout << "❌ Risk check failed - trade not allowed\n";
            }
        } else {
            std::cout << "🔍 No suitable weak points found\n";
        }
    }
    std::cout << "💀 Strategy terminated with honor\n";
}

int main() {
    try {
        std::cout << "🚀 Initializing trading system...\n";


       // Boost.Asio setup
        net::io_context ioc;
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths(); // Load system default SSL certificates

        std::string symbol = "btcusdt"; // The trading symbol

        // 1. Initialize BinanceWSClient
        BinanceWSClient client(ioc, ctx, symbol);
        MarketData& live_market_data = client.get_market_data();

        // 2. Initialize OrderBook
        OrderBook book;

        // 3. Get initial order book snapshot (blocking REST API call)
        std::cout << "Fetching initial order book snapshot via REST API...\n";
        auto snapshot_orders = client.get_snapshot(1000); // Get a deep snapshot (e.g., 1000 levels)
        book.initialize(snapshot_orders);
        std::cout << "Order book initialized with " << snapshot_orders.size() << " levels from snapshot.\n";
        std::cout << "Initial BBO: (" << std::fixed << std::setprecision(5) << book.get_bbo().first
                  << ", " << std::fixed << std::setprecision(5) << book.get_bbo().second << ")\n";

        // 4. Start WebSocket for live best bid/ask and depth updates
        client.start();
        std::cout << "🔌 BinanceWSClient started for live updates.\n";

        // 5. Initialize MarketPhaseDetector
        MarketPhaseDetector phase_detector;

        // 6. Start strategy thread(s)
        std::cout << "🧵 Starting strategy thread(s)...\n";
        std::vector<std::jthread> strategies;
        strategies.emplace_back([&]() {
            liquid_blood(live_market_data, book, phase_detector);
        });

        // 7. Optional: Start a separate market monitor thread to print updates
        std::jthread market_monitor([&](std::stop_token st) {
             while (!st.stop_requested()) {
                 MarketDataSnapshot updates_snapshot = live_market_data.get_updates();

                 {
                     std::lock_guard<std::mutex> lock(cout_mutex);
                     std::cout << "\n--- LIVE MARKET DATA ---\n";
                     if (updates_snapshot.connected) {
                         std::cout << "Status: CONNECTED\n";
                         std::cout << "  Best Bid: " << std::fixed << std::setprecision(5) << updates_snapshot.bid << "\n";
                         std::cout << "  Best Ask: " << std::fixed << std::setprecision(5) << updates_snapshot.ask << "\n";
                         std::cout << "  Timestamp: " << updates_snapshot.timestamp << " ms\n";
                         std::cout << "  Recent Depth Updates: " << updates_snapshot.recent_depth_updates.size() << " levels\n";
                         // Optionally print a few of the recent depth updates
                         /*
                         for (size_t i = 0; i < std::min((size_t)5, updates_snapshot.recent_depth_updates.size()); ++i) {
                             const auto& order = updates_snapshot.recent_depth_updates[i];
                             std::cout << "    " << (order.is_bid ? "BID" : "ASK")
                                       << " Price: " << std::fixed << std::setprecision(5) << static_cast<double>(order.price)
                                       << ", Amount: " << std::fixed << std::setprecision(5) << static_cast<double>(order.amount) << "\n";
                         }
                         */
                     } else {
                         std::cout << "Status: DISCONNECTED\n";
                     }
                 }
                 std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Print every 500ms
             }
         });


        std::cout << "🔥 Trading system online (Sun Tzu protocol engaged)\n";
        std::cout << "🛑 Press Ctrl+C to shutdown gracefully\n";

        // Set up signal handler for graceful shutdown
        std::signal(SIGINT, [](int) {
            std::cout << "\n🛑 SIGINT received - initiating shutdown...\n";
            global_blood_moon = true;
        });

        // 8. Run IO context in its own thread
        std::thread io_context_thread([&ioc]() {
            ioc.run();
            std::cout << "[main] IO context thread finished.\n";
        });


        // 9. Main loop for emergency stop monitoring
        while (!global_blood_moon) {
            static double initialDailyBalance = 1000.0;
            static double currentBalance = 1000.0;

            float loss_pct = RiskManager::getDailyLossPercent(initialDailyBalance, currentBalance);

            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "\n[MAIN] Daily P&L: " << (currentBalance - initialDailyBalance)
                          << " (" << std::fixed << std::setprecision(2) << loss_pct << "%)\n";
            }


            if (RiskManager::shouldStopTrading(loss_pct)) {
                std::cerr << "⚔️ Daily loss limit reached! Shutting down...\n";
                global_blood_moon = true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // --- Graceful shutdown sequence ---
        std::cout << "\n🛑 Shutting down system...\n";

        market_monitor.request_stop();
        market_monitor.join();

        client.stop();

        ioc.stop();

        if (io_context_thread.joinable()) {
            io_context_thread.join();
        }

        strategies.clear();

        std::cout << "🎋 Campaign concluded successfully\n";

    } catch (const std::exception& e) {
        std::cerr << "💥 FATAL ERROR: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}