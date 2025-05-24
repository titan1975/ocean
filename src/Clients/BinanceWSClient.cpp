#include "Clients/BinanceWSClient.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <boost/asio/strand.hpp>
#include <openssl/err.h>
#include <utility>
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
          symbol_(symbol) {
        std::cout << "[CONSTRUCTOR] BinanceWSClient created for symbol: " << symbol_ << std::endl;
    }

    void run() {
        std::cout << "[CONNECTING] Starting connection to Binance WS..." << std::endl;
        resolver_.async_resolve(
            "stream.binance.com",
            "9443",
            beast::bind_front_handler(
                &Impl::on_resolve,
                this));
    }

    void stop() {
        std::cout << "[SHUTDOWN] Closing WebSocket connection..." << std::endl;
        ws_.async_close(
            websocket::close_code::normal,
            [this](beast::error_code ec) {
                if (ec) {
                    std::cerr << "[ERROR] Close failed: " << ec.message() << std::endl;
                } else {
                    std::cout << "[SHUTDOWN] Connection closed gracefully" << std::endl;
                }
            });
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
        if (ec) {
            std::cerr << "[RESOLVE ERROR] " << ec.message() << std::endl;
            return handle_error(ec, "resolve");
        }

        std::cout << "[CONNECTING] Resolved IPs: ";
        for (const auto& entry : results) {
            std::cout << entry.endpoint() << " ";
        }
        std::cout << std::endl;

        net::async_connect(
            beast::get_lowest_layer(ws_),
            results,
            beast::bind_front_handler(
                &Impl::on_connect,
                this));
    }

    void on_connect(beast::error_code ec, tcp::endpoint ep) {
        if (ec) {
            std::cerr << "[CONNECT ERROR] " << ec.message() << std::endl;
            return handle_error(ec, "connect");
        }

        std::cout << "[CONNECTED] TCP connection established to " << ep << std::endl;

        if (!SSL_set_tlsext_host_name(
                ws_.next_layer().native_handle(),
                "stream.binance.com")) {
            ec = beast::error_code(
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category());
            std::cerr << "[SSL ERROR] SNI configuration failed" << std::endl;
            return handle_error(ec, "SSL_set_tlsext_host_name");
        }

        ws_.next_layer().async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(
                &Impl::on_ssl_handshake,
                this));
    }

    void on_ssl_handshake(beast::error_code ec) {
        if (ec) {
            std::cerr << "[SSL HANDSHAKE ERROR] " << ec.message() << std::endl;
            char buf[256];
            ERR_error_string_n(ec.value(), buf, sizeof(buf));
            std::cerr << "[SSL DETAILS] " << buf << std::endl;
            return handle_error(ec, "ssl_handshake");
        }

        std::cout << "[SECURE] SSL handshake successful" << std::endl;

        // Set suggested timeout settings for the websocket
        ws_.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client));

        ws_.async_handshake(
            "stream.binance.com",
            "/ws/" + symbol_ + "@depth20@100ms",
            beast::bind_front_handler(
                &Impl::on_handshake,
                this));
    }

    void on_handshake(beast::error_code ec) {
        if (ec) {
            std::cerr << "[HANDSHAKE ERROR] " << ec.message() << std::endl;

            // Alternative way to get error information without get_response()
            std::cerr << "[HANDSHAKE DETAILS] Error code: " << ec.value()
                      << ", Category: " << ec.category().name() << std::endl;

            return handle_error(ec, "handshake");
        }

        std::cout << "[CONNECTED] WebSocket handshake successful!" << std::endl;
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
                std::cout << "[CLOSED] Connection closed by server" << std::endl;
                if (ws_.reason().code != websocket::close_code::none) {
                    std::cout << "  Close code: " << ws_.reason().code << std::endl;
                    std::cout << "  Reason: " << ws_.reason().reason << std::endl;
                }
            } else {
                std::cerr << "[READ ERROR] " << ec.message() << std::endl;
            }
            return handle_error(ec, "read");
        }

        try {
            auto msg = beast::buffers_to_string(buffer_.data());
            json data = json::parse(msg);
            buffer_.consume(buffer_.size());

            // Full JSON dump
            // std::cout << "\n[RAW MESSAGE] " << std::string(50, '-') << "\n"
            //           << std::setw(4) << data << "\n"
            //           << std::string(50, '-') << std::endl;

            // Process market data
            {
                std::lock_guard<std::mutex> lock(data_.mutex);
                data_.bid = std::stod(data["bids"][0][0].get<std::string>());
                data_.ask = std::stod(data["asks"][0][0].get<std::string>());
                data_.timestamp = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                    .count());

                // std::cout << "[MARKET DATA] "
                //           << "Bid: " << data_.bid << " | "
                //           << "Ask: " << data_.ask << " | "
                //           << "Time: " << data_.timestamp << std::endl;
            }

        } catch (const std::exception& e) {
            std::cerr << "[PARSE ERROR] " << e.what() << std::endl;
            std::cerr << "[RAW DATA] " << beast::buffers_to_string(buffer_.data()) << std::endl;
            buffer_.consume(buffer_.size());
        }

        do_read();
    }

    void handle_error(beast::error_code ec, const char* what) {
        std::cerr << "[ERROR] " << what << " failed: "
                  << ec.message() << " (code: " << ec.value()
                  << ", category: " << ec.category().name() << ")" << std::endl;

        std::lock_guard<std::mutex> lock(data_.mutex);
        data_.connected = false;
    }
};

BinanceWSClient::BinanceWSClient(
    net::io_context& ioc,
    ssl::context& ctx,
    const std::string& symbol,
    int depth_level,
    const std::string& update_speed
) : pimpl_(std::make_unique<Impl>(ioc, ctx, symbol)) {
    // Your initialization code
}




BinanceWSClient::~BinanceWSClient() {
    std::cout << "[DESTRUCTOR] Cleaning up..." << std::endl;
    pimpl_->stop();
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

std::vector<OrderBook::Order> BinanceWSClient::get_snapshot(int depth) {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    namespace ssl = net::ssl;

    try {
        // 1. Create I/O context
        net::io_context ioc;

        // 2. Create SSL context
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();

        // 3. Create resolver and stream
        tcp::resolver resolver{ioc};
        beast::ssl_stream<beast::tcp_stream> stream{ioc, ctx};

        // 4. Set SNI hostname
        if(!SSL_set_tlsext_host_name(stream.native_handle(), "api.binance.com")) {
            throw beast::system_error(
                beast::error_code(
                    static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()),
                "Failed to set SNI Hostname");
        }

        // 5. Resolve and connect
        auto const results = resolver.resolve("api.binance.com", "443");
        beast::get_lowest_layer(stream).connect(results);

        // 6. SSL handshake
        stream.handshake(ssl::stream_base::client);

        // 7. Prepare HTTP request
        std::string target = "/api/v3/depth?symbol=" + symbol_ +
                           "&limit=" + std::to_string(depth);
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, "api.binance.com");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // 8. Send HTTP request
        http::write(stream, req);

        // 9. Receive HTTP response
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        // 10. Parse JSON
        nlohmann::json json = nlohmann::json::parse(beast::buffers_to_string(res.body().data()));

        // 11. Graceful shutdown
        beast::error_code ec;
        stream.shutdown(ec);
        if(ec == net::error::eof) {
            ec = {};
        }
        if(ec) {
            throw beast::system_error(ec);
        }

        return parse_snapshot(json);
    } catch(const std::exception& e) {
        std::cerr << "Snapshot error: " << e.what() << std::endl;
        throw;
    }
}

std::vector<OrderBook::Order> BinanceWSClient::parse_snapshot(const nlohmann::json& json) {
    std::vector<OrderBook::Order> snapshot;

    // Process bids
    for(auto& bid : json["bids"]) {
        snapshot.emplace_back(OrderBook::Order{
            .price = std::stof(bid[0].get<std::string>()),
            .amount = std::stof(bid[1].get<std::string>()),
            .is_bid = true
        });
    }

    // Process asks
    for(auto& ask : json["asks"]) {
        snapshot.emplace_back(OrderBook::Order{
            .price = std::stof(ask[0].get<std::string>()),
            .amount = std::stof(ask[1].get<std::string>()),
            .is_bid = false
        });
    }

    return snapshot;
}