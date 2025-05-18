#include "Clients/BinanceWSClient.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>
#include <boost/asio/strand.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

class BinanceWSClient::Impl {
public:
    Impl(net::io_context& ioc, ssl::context& ctx, const std::string& symbol)
        : resolver_(net::make_strand(ioc)),
          ws_(net::make_strand(ioc), ctx),
          symbol_(symbol) {}

    void run() {
        resolver_.async_resolve(
            "stream.binance.com",
            "9443",
            beast::bind_front_handler(
                &Impl::on_resolve,
                this));
    }

    void stop() {
        beast::error_code ec;
        ws_.async_close(
            websocket::close_code::normal,
            [](beast::error_code) {});
    }

    MarketData& get_data() noexcept {
        return data_;
    }

private:
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<tcp::socket>> ws_;
    MarketData data_;
    std::string symbol_;
    beast::flat_buffer buffer_;

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) return handle_error(ec, "resolve");

        net::async_connect(
            beast::get_lowest_layer(ws_),
            results,
            beast::bind_front_handler(
                &Impl::on_connect,
                this));
    }


    void on_connect(beast::error_code ec, tcp::endpoint) {
        if (ec) return handle_error(ec, "connect");

        if (!SSL_set_tlsext_host_name(
                ws_.next_layer().native_handle(),
                "stream.binance.com")) {
            ec = beast::error_code(
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category());
            return handle_error(ec, "SSL_set_tlsext_host_name");
        }

        ws_.next_layer().async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(
                &Impl::on_ssl_handshake,
                this));
    }

    void on_ssl_handshake(beast::error_code ec) {
        if (ec) return handle_error(ec, "ssl_handshake");

        ws_.async_handshake(
            "stream.binance.com",
            "/ws/" + symbol_ + "@depth20@100ms",
            beast::bind_front_handler(
                &Impl::on_handshake,
                this));
    }

    void on_handshake(beast::error_code ec) {
        if (ec) return handle_error(ec, "handshake");

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

    void on_read(beast::error_code ec, std::size_t) {
        if (ec) return handle_error(ec, "read");

        try {
            json data = json::parse(beast::buffers_to_string(buffer_.data()));
            buffer_.consume(buffer_.size());

            std::lock_guard<std::mutex> lock(data_.mutex);
            data_.bid = std::stod(data["bids"][0][0].get<std::string>());
            data_.ask = std::stod(data["asks"][0][0].get<std::string>());
            data_.timestamp = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                .count());
        } catch (const std::exception& e) {
            std::cerr << "JSON error: " << e.what() << '\n';
        }

        do_read();
    }

    void handle_error(beast::error_code ec, const char* what) {
        std::cerr << what << ": " << ec.message() << '\n';
        std::lock_guard<std::mutex> lock(data_.mutex);
        data_.connected = false;
    }
};

BinanceWSClient::BinanceWSClient(net::io_context& ioc, ssl::context& ctx, const std::string& symbol)
    : pimpl_(std::make_unique<Impl>(ioc, ctx, symbol)) {}

BinanceWSClient::~BinanceWSClient() {
    stop();
}

void BinanceWSClient::start() {
    pimpl_->run();
}

void BinanceWSClient::stop() {
    pimpl_->stop();
}

BinanceWSClient::MarketData& BinanceWSClient::get_market_data() noexcept {
    return pimpl_->get_data();
}

