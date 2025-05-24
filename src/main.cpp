#include "iostream"
#include "thread"
#include "atomic"
#include "csignal"
#include "vector"
#include "memory"
#include <mutex>


#include "Strategy/LiquidityRaidDetector.hpp"
#include "Risk/RiskManager.hpp"
#include "Tactics/SunTzuTactics.hpp"
#include "Analysis/MarketPhaseDetector.hpp"
#include "Clients/BinanceWSClient.hpp"
#include "Core/MarketData.hpp"


class MarketPhaseDetector;
class OrderBook;
class MarketData;
// Global kill switch with Sun Tzu wisdom
std::atomic<bool> global_blood_moon{false};
std::mutex cout_mutex;
//------------------------------------------------------------------
// L I Q U I D   B L O O D   S T R A T E G Y  (SUN TZU EDITION)
//------------------------------------------------------------------
void liquid_blood(MarketData& market, OrderBook& book, MarketPhaseDetector& phase_detector) {
    std::cout << "🟢 Starting Liquid Blood strategy thread\n";

    LiquidityRaidDetector raid_detector({
        .volume_spike_multiplier = 2.5f,
        .min_wick_ratio = 1.8f
    });
    std::cout << "🔧 Liquidity Raid Detector configured: "
              << "Volume Spike=" << 2.5f << ", Min Wick Ratio=" << 1.8f << "\n";

    while (!global_blood_moon) {
        std::cout << "\n--- NEW STRATEGY CYCLE ---\n";

        // 1. Get Market Data
        std::cout << "🔄 Fetching market updates...\n";
        auto updates = market.get_updates();
        std::cout << "📊 Received " << updates.size() << " market updates\n";

        // 2. Update Market Phase
        float mid_price = book.get_mid_price();
        std::cout << "📈 Current mid price: " << mid_price << "\n";
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
        SunTzu::adjustForMarketPhase(phase);

        if (updates.empty()) {
            std::cout << "⏳ No updates available, sleeping 1ms...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // 4. Check Trading Conditions
        std::cout << "🔍 Checking for weak points in order book...\n";
        if (SunTzu::isWeakPoint(book, 5000.0f)) {
            std::cout << "🎯 Weak point detected! Preparing trade...\n";

            // 5. Calculate Entry
            float entry = SunTzu::stealthEntryPrice(book, true);
            float stop_loss = entry * 0.95f;
            std::cout << "💰 Entry Price: " << entry
                      << " | Stop Loss: " << stop_loss << " (" << (entry-stop_loss) << " risk)\n";

            // 6. Risk Management
            double balance = RiskManager::getAccountBalance();
            std::cout << "💳 Account Balance: " << balance << "\n";
            double size = RiskManager::calculatePositionSize(entry, stop_loss, balance);
            std::cout << "📏 Calculated Position Size: " << size << "\n";

            if (RiskManager::isTradeAllowed(0.01)) {
                std::cout << "✅ Risk check passed (1% risk allowed)\n";

                // 7. Update Order Book
                std::cout << "📚 Updating order book with new data...\n";
                book.update(std::vector<OrderBook::Order>(updates.begin(), updates.end()));

                // 8. Check for Raids
                if (raid_detector.detect_raid(updates, mid_price)) {
                    std::cout << "⚡ RAID DETECTED! EXECUTING TRADE:\n";
                    std::cout << "   ➡️ Entry: " << entry << "\n";
                    std::cout << "   🛑 Stop: " << stop_loss << "\n";
                    std::cout << "   📊 Size: " << size << "\n";
                    // TODO: Actual execution would go here
                } else {
                    std::cout << "🔍 No liquidity raid detected\n";
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
        std::cout << "🔌 Connecting to BTC/USDT market...\n";

        net::io_context ioc;
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();

        // 1. Initialize client
        BinanceWSClient client(ioc, ctx, "btcusdt");


        // 2. Get initial snapshot (blocking call)
        std::cout << "Fetching order book snapshot...\n";
        OrderBook book;
        auto snapshot = client.get_snapshot();
        book.initialize(snapshot);
        std::cout << "Order book initialized with " << snapshot.size() << " levels\n";

        // 3. Start WebSocket for live updates
        client.start();


        // 4. Start strategy thread
        MarketPhaseDetector phase_detector;
        std::jthread strategy([&] {
            liquid_blood(client.get_market_data(), book, phase_detector);
        });

        // 5. Run IO context in main thread
        ioc.run();

        MarketData market("btcusdt");

        if (!market.start()) {
            throw std::runtime_error("❌ Market data connection failed");
        }
        std::cout << "✅ Market data connected successfully\n";

        OrderBook book;
        std::cout << "📖 Initializing order book...\n";

        market.get_snapshot();
        MarketPhaseDetector phase_detector;

        std::cout << "🧵 Starting strategy thread...\n";
        std::vector<std::jthread> strategies;
        strategies.emplace_back([&] {
            liquid_blood(market, book, phase_detector);
        });

        std::jthread market_monitor([&](std::stop_token st) {
     while (!st.stop_requested()) {
         auto updates = market.get_updates();

         {
             std::lock_guard<std::mutex> lock(cout_mutex);
             std::cout << "\n--- MARKET UPDATE ---\n";
             for (const auto& order : updates) {
                 std::cout << (order.is_bid ? "🟢 BID" : "🔴 ASK")
                           << " | Price: " << std::setw(10) << std::setprecision(8)
                           << static_cast<double>(order.price)
                           << " | Amount: " << std::setw(10) << std::setprecision(8)
                           << static_cast<double>(order.amount) << "\n";
             }
         }

         std::this_thread::sleep_for(std::chrono::milliseconds(100));
     }
 });

        std::cout << "🔥 Trading system online (Sun Tzu protocol engaged)\n";
        std::cout << "🛑 Press Ctrl+C to shutdown gracefully\n";

        std::signal(SIGINT, [](int) {
            std::cout << "\n🛑 SIGINT received - initiating shutdown...\n";
            global_blood_moon = true;
        });

        // Emergency stop monitoring
        while (!global_blood_moon) {
            double initialDailyBalance = 300;
            double currentBalance = 350;
            float loss_pct = RiskManager::getDailyLossPercent(initialDailyBalance, currentBalance);

            std::cout << "📉 Daily P&L: " << (currentBalance - initialDailyBalance)
                      << " (" << loss_pct << "%)\n";

            if (RiskManager::shouldStopTrading(loss_pct)) {
                std::cerr << "⚔️ Daily loss limit reached! Shutting down...\n";
                global_blood_moon = true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Graceful shutdown
        std::cout << "🛑 Shutting down system...\n";
        market.stop();
        market_monitor.request_stop();
        strategies.clear();
        std::cout << "🎋 Campaign concluded successfully\n";

    } catch (const std::exception& e) {
        std::cerr << "💥 FATAL ERROR: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}