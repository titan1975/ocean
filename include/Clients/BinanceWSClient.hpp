#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <iostream>  // Added for std::cerr
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>  // Added for tcp
#include <boost/asio/ssl.hpp>

namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;  // Explicit tcp definition

class BinanceWSClient {
public:
    struct MarketData {
        std::mutex mutex;
        double bid = 0.0;
        double ask = 0.0;
        uint64_t timestamp = 0;
        bool connected = false;
    };

    BinanceWSClient(net::io_context& ioc, ssl::context& ctx, const std::string& symbol);
    ~BinanceWSClient();

    void start();
    void stop();
    MarketData& get_market_data() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};