#pragma once

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>
#include <mutex>
#include <atomic>
#include <memory>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

class BinanceWSClient {
public:
    struct MarketData {
        double bid = 0.0;
        double ask = 0.0;
        uint64_t timestamp = 0;
        bool connected = false;
        std::mutex mutex;
    };

    BinanceWSClient(
        net::io_context& ioc,
        ssl::context& ctx,
        const std::string& symbol,
        int depth_level = 20,
        const std::string& update_speed = "100ms"
    );
    ~BinanceWSClient();

    void start();
    void stop();
    MarketData& get_market_data() noexcept;

    BinanceWSClient(const BinanceWSClient&) = delete;
    BinanceWSClient& operator=(const BinanceWSClient&) = delete;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};