#pragma once
#include "Core/OrderBook.hpp"
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
    struct MarketData {
        double bid = 0.0;
        double ask = 0.0;
        uint64_t timestamp = 0;
        bool connected = false;
        std::mutex mutex;
    };

    BinanceWSClient(net::io_context& ioc, ssl::context& ctx, const std::string& symbol);
    ~BinanceWSClient();

    void start();
    void stop();
    MarketData& get_market_data() noexcept;
    std::vector<OrderBook::Order> get_snapshot(int depth = 1000);

    BinanceWSClient(const BinanceWSClient&) = delete;
    BinanceWSClient& operator=(const BinanceWSClient&) = delete;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    std::string symbol_;
};