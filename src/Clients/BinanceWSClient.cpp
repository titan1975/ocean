#include "Clients/BinanceWSClient.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>
#include <boost/asio/strand.hpp>
#include <openssl/err.h>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

// ====================== Impl ======================
// 🕶️ "The real work happens here (while you sleep)"
class BinanceWSClient::Impl {
public:
    Impl(net::io_context& ioc, ssl::context& ctx, const std::string& symbol)
        : resolver_(net::make_strand(ioc)),
          ws_(net::make_strand(ioc), ctx),
          symbol_(symbol) {
        std::cout << "🔌 [Impl] Constructing for symbol: " << symbol_ << std::endl;
    }

    void run() {
        std::cout << "🌐 [Impl] Resolving Binance WS endpoint..." << std::endl;
        resolver_.async_resolve(
            "stream.binance.com",
            "9443",
            beast::bind_front_handler(
                &Impl::on_resolve,
                this));
    }

    void stop() {
        std::cout << "💀 [Impl] Closing WebSocket..." << std::endl;
        ws_.async_close(
            websocket::close_code::normal,
            [this](beast::error_code ec) {
                if (ec) {
                    std::cerr << "❌ [Impl] Close error: " << ec.message() << std::endl;
                }
            });
    }

    MarketData& get_data() noexcept {
        return data_;  // 📦 "Your data, sir/madam/attack helicopter"
    }

private:
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<tcp::socket>> ws_;
    MarketData data_;
    std::string symbol_;
    beast::flat_buffer buffer_;

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) {
            std::cerr << "🔴 [Impl] Resolve error: " << ec.message() << std::endl;
            return handle_error(ec, "resolve");
        }

        std::cout << "🛰️ [Impl] Resolved IPs. Connecting TCP..." << std::endl;

        // CORRECT for Boost 1.74 - use the endpoint iterator version
        net::async_connect(
            beast::get_lowest_layer(ws_),
            results,
            beast::bind_front_handler(&Impl::on_connect, this)
        );
    }



    void on_connect(beast::error_code ec, tcp::endpoint ep) {
        if (ec) {
            std::cerr << "🔴 [Impl] Connect error: " << ec.message() << std::endl;
            return handle_error(ec, "connect");
        }

        std::cout << "🔐 [Impl] TCP connected. SSL handshaking..." << std::endl;
        if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), "stream.binance.com")) {
            ec = beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
            return handle_error(ec, "SNI");
        }

        ws_.next_layer().async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(
                &Impl::on_ssl_handshake,
                this));
    }

    void on_ssl_handshake(beast::error_code ec) {
        if (ec) {
            std::cerr << "🔴 [Impl] SSL handshake error: " << ec.message() << std::endl;
            return handle_error(ec, "ssl_handshake");
        }

        std::cout << "🤝 [Impl] SSL secured! WebSocket handshaking..." << std::endl;
        ws_.async_handshake(
            "stream.binance.com",
            "/ws/" + symbol_ + "@depth20@100ms",
            beast::bind_front_handler(
                &Impl::on_handshake,
                this));
    }

    void on_handshake(beast::error_code ec) {
        if (ec) {
            std::cerr << "🔴 [Impl] WS handshake error: " << ec.message() << std::endl;
            return handle_error(ec, "handshake");
        }

        std::cout << "🚀 [Impl] WebSocket CONNECTED! Streaming data..." << std::endl;
        {
            std::lock_guard<std::mutex> lock(data_.mutex);
            data_.connected = true;
        }
        do_read();
    }

    void do_read() {
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &Impl::on_read,
                this));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
            if (ec == websocket::error::closed) {
                std::cout << "🛑 [Impl] Server closed connection gracefully." << std::endl;
            } else {
                std::cerr << "🔴 [Impl] Read error: " << ec.message() << std::endl;
            }
            return handle_error(ec, "read");
        }

        try {
            auto msg = beast::buffers_to_string(buffer_.data());
            json data = json::parse(msg);
            buffer_.consume(buffer_.size());

            {
                std::lock_guard<std::mutex> lock(data_.mutex);
                data_.bid = std::stod(data["bids"][0][0].get<std::string>());
                data_.ask = std::stod(data["asks"][0][0].get<std::string>());
                data_.timestamp = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                    .count());
            }
        } catch (const std::exception& e) {
            std::cerr << "🔴 [Impl] JSON parse error: " << e.what() << std::endl;
        }

        do_read();  // 🔁 "Read it again, Sam"
    }

    void handle_error(beast::error_code ec, const char* what) {
        std::cerr << "💥 [Impl] ERROR in " << what << ": " << ec.message() << std::endl;
        std::lock_guard<std::mutex> lock(data_.mutex);
        data_.connected = false;
    }
};

// ====================== BinanceWSClient ======================
// 🏗️ Constructor: "I LIVE!"
BinanceWSClient::BinanceWSClient(net::io_context& ioc, ssl::context& ctx, const std::string& symbol)
    : pimpl_(std::make_unique<Impl>(ioc, ctx, symbol)), symbol_(symbol) {}

// 💀 Destructor: "I die as I lived—dramatically."
BinanceWSClient::~BinanceWSClient() { pimpl_->stop(); }

// 🎬 Start: "ACTION!"
void BinanceWSClient::start() { pimpl_->run(); }

// 🛑 Stop: "CUT! That's a wrap."
void BinanceWSClient::stop() { pimpl_->stop(); }

// 📊 Market Data: "Here, have some fresh data!"
BinanceWSClient::MarketData& BinanceWSClient::get_market_data() noexcept {
    return pimpl_->get_data();
}

// 📸 Snapshot: "Say cheese!" (but for order books)
std::vector<OrderBook::Order> BinanceWSClient::get_snapshot(int depth) {
    namespace http = beast::http;
    try {
        net::io_context ioc;
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();

        tcp::resolver resolver{ioc};
        beast::ssl_stream<beast::tcp_stream> stream{ioc, ctx};

        if (!SSL_set_tlsext_host_name(stream.native_handle(), "api.binance.com")) {
            throw beast::system_error(
                beast::error_code(
                    static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()),
                "SNI fail");
        }

        auto const results = resolver.resolve("api.binance.com", "443");
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::get,
            "/api/v3/depth?symbol=" + symbol_ + "&limit=" + std::to_string(depth), 11};
        req.set(http::field::host, "api.binance.com");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        json data = json::parse(beast::buffers_to_string(res.body().data()));

        beast::error_code ec;
        stream.shutdown(ec);
        if (ec == net::error::eof) ec = {};
        if (ec) throw beast::system_error(ec);

        std::vector<OrderBook::Order> snapshot;
        for (auto& bid : data["bids"]) {
            snapshot.emplace_back(OrderBook::Order{
                std::stof(bid[0].get<std::string>()),
                std::stof(bid[1].get<std::string>()),
                true  // is_bid
            });
        }
        for (auto& ask : data["asks"]) {
            snapshot.emplace_back(OrderBook::Order{
                std::stof(ask[0].get<std::string>()),
                std::stof(ask[1].get<std::string>()),
                false  // is_bid
            });
        }
        return snapshot;
    } catch (const std::exception& e) {
        std::cerr << "💥 [Snapshot] ERROR: " << e.what() << std::endl;
        throw;
    }
}