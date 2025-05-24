#pragma once
#include "Core/OrderBook.hpp"  // 🦆 OrderBook duck-typing its way in
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>
#include <mutex>
#include <memory>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

class BinanceWSClient {
public:
    // 📊 MarketData struct: Where bids and asks fight like cats and dogs 🐱🐶
    struct MarketData {
        double bid = 0.0;       // 🤑 "I'll buy that for a dollar!"
        double ask = 0.0;       // 🏷️ "Price tag says..."
        uint64_t timestamp = 0; // ⏰ "Time is money, friend."
        bool connected = false; // 🔌 "Did someone unplug the server?"
        std::mutex mutex;      // 🔒 "STOP RIGHT THERE, CRIMINAL SCUM!"
    };

    // 🏗️ Constructor: Building the BinanceWSClient like a LEGO set
    BinanceWSClient(net::io_context& ioc, ssl::context& ctx, const std::string& symbol);

    // 💣 Destructor: "I'll be back... (to clean up)" - Terminator
    ~BinanceWSClient();

    // 🚀 Start: "3... 2... 1... LIFTOFF!"
    void start();

    // 🛑 Stop: "Pull the plug!"
    void stop();

    // 📈 Get Market Data: "Show me the money!" - Jerry Maguire
    MarketData& get_market_data() noexcept;

    // 📸 Get Snapshot: "Say cheese!" 📸 (but for order books)
    std::vector<OrderBook::Order> get_snapshot(int depth = 1000);

    // 🚫 Copying forbidden! "This is SPARTA!" (kicks copy constructors into the pit)
    BinanceWSClient(const BinanceWSClient&) = delete;
    BinanceWSClient& operator=(const BinanceWSClient&) = delete;

private:
    // 🕵️‍♂️ PIMPL idiom: "Nothing to see here, move along."
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    std::string symbol_;  // 💎 The chosen symbol (BTCUSDT, ETHUSDT, etc.)
};