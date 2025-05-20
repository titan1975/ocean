#include "Clients/BinanceWSClient.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <openssl/err.h>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <chrono>  // For std::chrono::seconds


class BinanceWSClient::Impl : public std::enable_shared_from_this<BinanceWSClient::Impl> {
public:
    Impl(
        net::io_context& ioc,
        ssl::context& ctx,
        const std::string& symbol,
        int depth_level,
        const std::string& update_speed
    ) : resolver_(net::make_strand(ioc)),
          ws_(net::make_strand(ioc), ctx),
          timer_(net::make_strand(ioc)),
          symbol_(symbol),
          depth_level_(depth_level),
          update_speed_(update_speed),
          reconnect_attempts_(0),
          stopping_(false) {
        std::cout << "[CONSTRUCTOR] BinanceWSClient for " << symbol_
                  << " (Depth: " << depth_level_ << ", Speed: " << update_speed_ << ")\n";
    }

    ~Impl() {
        stop();
    }

    void run() {
        if (stopping_.load()) return;
        std::cout << "[CONNECTING] Starting connection...\n";
        resolver_.async_resolve(
            "stream.binance.com",
            "9443",
            beast::bind_front_handler(
                &Impl::on_resolve,
                shared_from_this()));
    }

    void stop() {
        stopping_.store(true);
        timer_.cancel();

        net::post(
            ws_.get_executor(),
            [self = shared_from_this()]() {
                if (self->ws_.is_open()) {
                    self->ws_.async_close(
                        websocket::close_code::normal,
                        [](beast::error_code ec) {
                            if (ec) {
                                std::cerr << "[WARN] Close error: " << ec.message() << "\n";
                            }
                        });
                }
            });
    }

    MarketData& get_data() noexcept {
        return data_;
    }

private:
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<tcp::socket>> ws_;
    net::steady_timer timer_;
    MarketData data_;
    std::string symbol_;
    int depth_level_;
    std::string update_speed_;
    beast::flat_buffer buffer_;
    std::atomic<int> reconnect_attempts_{0};
    std::atomic<bool> stopping_{false};

    void schedule_reconnect() {
        if (stopping_.load()) return;

        const int max_retries = 5;
        const int base_delay_ms = 1000;
        int delay_ms = std::min(
            base_delay_ms * (1 << std::min(reconnect_attempts_.load(), max_retries)),
            30000
        );

        reconnect_attempts_++;
        std::cout << "[RECONNECT] Attempt " << reconnect_attempts_.load()
                  << " in " << delay_ms << "ms\n";

        timer_.expires_after(std::chrono::milliseconds(delay_ms));
        timer_.async_wait(
            [self = shared_from_this()](beast::error_code ec) {
                if (ec || self->stopping_.load()) return;
                self->run();
            });
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec || stopping_.load()) {
            std::cerr << "[RESOLVE ERROR] " << ec.message() << "\n";
            return schedule_reconnect();
        }

        std::cout << "[CONNECTING] Resolved IPs: ";
        for (const auto& entry : results) std::cout << entry.endpoint() << " ";
        std::cout << "\n";

        // ✅ FIXED: Proper timeout syntax on TCP layer
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(5));

        net::async_connect(
            beast::get_lowest_layer(ws_),
            results,
            beast::bind_front_handler(
                &Impl::on_connect,
                shared_from_this()));
    }



    void on_connect(beast::error_code ec, tcp::endpoint ep) {
        if (ec || stopping_.load()) {
            std::cerr << "[CONNECT ERROR] " << ec.message() << "\n";
            return schedule_reconnect();
        }

        std::cout << "[CONNECTED] TCP to " << ep << "\n";

        // ✅ FIXED: Disable timeout correctly
        beast::get_lowest_layer(ws_).expires_never();
        // ... (rest of the code remains the same)
    }

    void on_ssl_handshake(beast::error_code ec) {
        if (ec || stopping_.load()) {
            std::cerr << "[SSL HANDSHAKE ERROR] " << ec.message() << "\n";
            return schedule_reconnect();
        }

        std::cout << "[SECURE] SSL handshake OK\n";
        ws_.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client));

        const std::string stream = "/ws/" + symbol_ +
            "@depth" + std::to_string(depth_level_) + "@" + update_speed_;

        ws_.async_handshake(
            "stream.binance.com",
            stream,
            beast::bind_front_handler(
                &Impl::on_handshake,
                shared_from_this()));
    }

    void on_handshake(beast::error_code ec) {
        if (ec || stopping_.load()) {
            std::cerr << "[HANDSHAKE ERROR] " << ec.message() << "\n";
            return schedule_reconnect();
        }

        reconnect_attempts_ = 0;
        std::cout << "[CONNECTED] WebSocket OK\n";
        {
            std::lock_guard<std::mutex> lock(data_.mutex);
            data_.connected = true;
        }
        do_read();
    }

    void do_read() {
        if (stopping_.load()) return;
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &Impl::on_read,
                shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
            if (ec == websocket::error::closed) {
                std::cout << "[CLOSED] Server closed connection. Code: "
                          << ws_.reason().code << ", Reason: "
                          << ws_.reason().reason << "\n";
            } else {
                std::cerr << "[READ ERROR] " << ec.message() << "\n";
            }
            {
                std::lock_guard<std::mutex> lock(data_.mutex);
                data_.connected = false;
            }
            return schedule_reconnect();
        }

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
            std::cerr << "[PARSE ERROR] " << e.what() << "\n";
        }

        do_read();
    }
};

BinanceWSClient::BinanceWSClient(
    net::io_context& ioc,
    ssl::context& ctx,
    const std::string& symbol,
    int depth_level,
    const std::string& update_speed
) : pimpl_(std::make_unique<Impl>(ioc, ctx, symbol, depth_level, update_speed)) {}

BinanceWSClient::~BinanceWSClient() {
    std::cout << "[DESTRUCTOR] Shutting down...\n";
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