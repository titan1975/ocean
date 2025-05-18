#include "iostream"
#include "thread"
#include "atomic"
#include "csignal"
#include "vector"
#include "memory"



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

//------------------------------------------------------------------
// L I Q U I D   B L O O D   S T R A T E G Y  (SUN TZU EDITION)
//------------------------------------------------------------------
void liquid_blood(MarketData& market, OrderBook& book, MarketPhaseDetector& phase_detector) {
    LiquidityRaidDetector raid_detector({
        .volume_spike_multiplier = 2.5f,  // This should come first
           .min_wick_ratio = 1.8f
    });

    while (!global_blood_moon) {
        // Sun Tzu Principle: "Know the terrain"
        auto updates = market.get_updates();
        phase_detector.update(book.get_mid_price());
        SunTzu::adjustForMarketPhase(phase_detector.getPhase());

        if (updates.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // Sun Tzu Principle: "Attack only when strong"
        if (SunTzu::isWeakPoint(book, 5000.0f)) {
            // Stealth entry (Deception tactic)
            float entry = SunTzu::stealthEntryPrice(book, true);
            float stop_loss = entry * 0.95f; // 5% stop

            // Risk-managed position sizing
            double size = RiskManager::calculatePositionSize(
                entry,
                stop_loss,
                RiskManager::getAccountBalance()
            );

            // Execute only if risk parameters allow
            if (RiskManager::isTradeAllowed(0.01)) { // 1% risk
                book.update(std::vector<OrderBook::Order>(updates.begin(), updates.end()));

                if (raid_detector.detect_raid(updates, book.get_mid_price())) {
                    std::cout << "⚡ RAID DETECTED! ENTERING AT " << entry << "\n";
                    // TODO: Add execution logic with stealth orders
                }
            }
        }
    }
    std::cout << "💀 Strategy terminated with honor\n";
}

//------------------------------------------------------------------
// M A I N   W A R   R O O M
//------------------------------------------------------------------
int main() {
    try {

        net::io_context ioc;
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();

        BinanceWSClient btc_client(ioc, ctx, "btcusdt");
        btc_client.start();

        std::thread ioc_thread([&ioc]{
            ioc.run();
        });
        // Access data in your main loop
        while (true) {
            auto& data = btc_client.get_market_data();
            std::lock_guard<std::mutex> lock(data.mutex);
            if (data.connected) {



                std::cout << "BTC/USDT - Bid: " << data.bid
                          << " | Ask: " << data.ask
                          << " | TS: " << data.timestamp << '\n';
            } else {
                std::cout << "Disconnected\n";
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        ioc_thread.join();
        // Sun Tzu Principle: "Preparation determines victory"
        MarketData market("127.0.0.1", 1337);
        OrderBook book;
        MarketPhaseDetector phase_detector;

        if (!market.start()) {
            throw std::runtime_error("Market data connection failed");
        }

        // Sun Tzu Principle: "Divide your forces wisely"
        std::vector<std::jthread> strategies;
        strategies.emplace_back([&] {
            liquid_blood(market, book, phase_detector);
        });

        std::cout << "🔥 Trading system online (Sun Tzu protocol engaged)\n";

        // Sun Tzu Principle: "Know when to retreat"
        std::signal(SIGINT, [](int) {
            std::cout << "\n忍 (Enduring the retreat)\n";
            global_blood_moon = true;
        });
        double initialDailyBalance = 300;/* your initial daily balance */;
        double currentBalance = 350/* your current balance */;
        // Emergency stop conditions
        while (!global_blood_moon) {

            double initialDailyBalance = 300;/* your initial daily balance */;
            double currentBalance = 350/* your current balance */;

            if (RiskManager::shouldStopTrading(RiskManager::getDailyLossPercent(initialDailyBalance, currentBalance))) {
                std::cerr << "⚔️ Daily loss limit reached! Withdrawing!\n";
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Graceful shutdown
        market.stop();
        strategies.clear();
        std::cout << "🎋 Campaign concluded successfully\n";

    } catch (const std::exception& e) {
        std::cerr << "💥 STRATEGIC FAILURE: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// BinanceClient client("BTCUSDT");
// if (client.start()) {
//     while (true) {
//         auto updates = client.get_updates();
//         if (!updates.empty()) {
//             // Process updates
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
// }
// client.stop();