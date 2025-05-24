#include "Clients/BinanceWSClient.hpp"
#include "Core/MarketData.hpp" // Ensure MarketData.hpp is included for MarketData type
#include "Core/OrderBook.hpp"  // Ensure OrderBook.hpp is included for OrderBook::Order type

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/http.hpp> // Required for REST API calls
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>

#include <boost/asio/strand.hpp>
#include <openssl/err.h>
#include <utility> // For std::move

namespace beast = boost::beast;         // from <boost/beast/core.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio/io_context.hpp>
namespace ssl = net::ssl;               // from <boost/asio/ssl/context.hpp>
using tcp = net::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
using json = nlohmann::json;            // from <nlohmann/json.hpp>

// Define the Impl class within the .cpp file, as per PIMPL idiom
class BinanceWSClient::Impl {
public:
    Impl(net::io_context& ioc, ssl::context& ctx,
         const std::string& symbol, int depth_level, const std::string& update_speed)
        : resolver_(net::make_strand(ioc)),
          ws_(net::make_strand(ioc), ctx),
          symbol_(symbol),
          websocket_url_path_("/ws/" + symbol + "@depth" + std::to_string(depth_level) + "@" + update_speed),
          http_host_("api.binance.com"), // Host for REST API
          http_port_("443") // Port for HTTPS REST API
    {
        std::cout << "[CONSTRUCTOR] BinanceWSClient::Impl created for symbol: "
                  << symbol_ << ", WS Path: " << websocket_url_path_ << std::endl;
    }

    // --- WebSocket Methods ---

    void run() {
        std::cout << "[CONNECTING] Starting connection to Binance WS... (Path: " << websocket_url_path_ << ")" << std::endl;
        resolver_.async_resolve(
            "stream.binance.com", // WebSocket host
            "9443",               // WebSocket port
            beast::bind_front_handler(
                &Impl::on_resolve,
                this));
    }

    void stop() {
        std::cout << "[SHUTDOWN] Closing WebSocket connection..." << std::endl;
        if (ws_.is_open()) {
            ws_.async_close(
                websocket::close_code::normal,
                [this](beast::error_code ec) {
                    if (ec) {
                        std::cerr << "[ERROR] Close failed: " << ec.message() << std::endl;
                    } else {
                        std::cout << "[SHUTDOWN] Connection closed gracefully" << std::endl;
                    }
                    // Ensure connected status is false
                    data_.update_market_data(0.0, 0.0, 0, false, {}); // Update with disconnected status
                });
        } else {
            std::cout << "[SHUTDOWN] WebSocket not open, no close needed." << std::endl;
            data_.update_market_data(0.0, 0.0, 0, false, {}); // Update with disconnected status
        }
    }

    MarketData& get_data() noexcept {
        return data_;
    }

    // --- REST API Snapshot Method ---
    std::vector<OrderBook::Order> get_snapshot_sync(int depth) {
        std::vector<OrderBook::Order> orders;
        std::cout << "[SNAPSHOT] Fetching order book snapshot for " << symbol_ << " depth " << depth << std::endl;

        net::io_context ioc_snapshot; // Separate io_context for blocking HTTP request
        ssl::context ctx_snapshot{ssl::context::tlsv12_client};
        ctx_snapshot.set_default_verify_paths();

        tcp::resolver resolver(ioc_snapshot);
        beast::ssl_stream<tcp::socket> stream(ioc_snapshot, ctx_snapshot);
        beast::error_code ec;

        // Set SNI for SSL handshake
        if (!SSL_set_tlsext_host_name(stream.native_handle(), http_host_.c_str())) {
            ec = beast::error_code(
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category());
            std::cerr << "[SNAPSHOT ERROR] SSL_set_tlsext_host_name failed: " << ec.message() << std::endl;
            return {};
        }

        // Resolve host
        auto const results = resolver.resolve(http_host_, http_port_, ec);
        if (ec) {
            std::cerr << "[SNAPSHOT ERROR] Resolve failed: " << ec.message() << std::endl;
            return {};
        }

        // FIX: Use boost::asio::connect to handle resolver results
        // This function attempts to connect to all endpoints in the results list
        // until one succeeds.
        net::connect(beast::get_lowest_layer(stream), results, ec);
        if (ec) {
            std::cerr << "[SNAPSHOT ERROR] Connect failed: " << ec.message() << std::endl;
            return {};
        }

        // Perform SSL handshake
        stream.handshake(ssl::stream_base::client, ec);
        if (ec) {
            std::cerr << "[SNAPSHOT ERROR] SSL handshake failed: " << ec.message() << std::endl;
            return {};
        }

        // Set up the HTTP GET request
        http::request<http::string_body> req{http::verb::get, "/api/v3/depth?symbol=" + symbol_ + "&limit=" + std::to_string(depth), 11};
        req.set(http::field::host, http_host_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request
        http::write(stream, req, ec);
        if (ec) {
            std::cerr << "[SNAPSHOT ERROR] Write request failed: " << ec.message() << std::endl;
            return {};
        }

        // Receive the HTTP response
        beast::flat_buffer response_buffer;
        http::response<http::string_body> res;
        http::read(stream, response_buffer, res, ec);
        if (ec) {
            std::cerr << "[SNAPSHOT ERROR] Read response failed: " << ec.message() << std::endl;
            return {};
        }

        // Close the stream
        stream.shutdown(ec);
        if (ec == net::error::eof || ec == ssl::error::stream_truncated) {
            // Rationale: http://www.boost.org/doc/libs/1_80_0/libs/beast/doc/html/beast/using_http/graceful_shutdown.html
            ec = {}; // Clear error if it's a graceful shutdown indication
        }
        if (ec) {
            std::cerr << "[SNAPSHOT ERROR] Shutdown failed: " << ec.message() << std::endl;
            return {};
        }

        // Parse the JSON response
        try {
            json snapshot_json = json::parse(res.body());

            // Process bids
            if (snapshot_json.contains("bids") && snapshot_json["bids"].is_array()) {
                for (const auto& bid_level : snapshot_json["bids"]) {
                    if (bid_level.is_array() && bid_level.size() >= 2) {
                        orders.emplace_back(std::stod(bid_level[0].get<std::string>()),
                                           std::stod(bid_level[1].get<std::string>()),
                                           true); // is_bid = true
                    }
                }
            }
            // Process asks
            if (snapshot_json.contains("asks") && snapshot_json["asks"].is_array()) {
                for (const auto& ask_level : snapshot_json["asks"]) {
                    if (ask_level.is_array() && ask_level.size() >= 2) {
                        orders.emplace_back(std::stod(ask_level[0].get<std::string>()),
                                           std::stod(ask_level[1].get<std::string>()),
                                           false); // is_bid = false
                    }
                }
            }


        } catch (const std::exception& e) {
            std::cerr << "[SNAPSHOT PARSE ERROR] " << e.what() << "\n" << res.body() << std::endl;
            return {};
        }

        std::cout << "[SNAPSHOT] Fetched " << orders.size() << " order book levels." << std::endl;
        return orders;
    }


private:
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<tcp::socket>> ws_;
    MarketData data_; // Our MarketData instance
    std::string symbol_;
    std::string websocket_url_path_; // Dynamically constructed WS path
    std::string http_host_;          // Host for REST API
    std::string http_port_;          // Port for REST API
    beast::flat_buffer buffer_;

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) {
            std::cerr << "[RESOLVE ERROR] " << ec.message() << std::endl;
            return handle_error(ec, "resolve");
        }

        std::cout << "[CONNECTING] Resolved IPs for WS: ";
        for (const auto& entry : results) {
            std::cout << entry.endpoint() << " ";
        }
        std::cout << std::endl;

        // Use boost::asio::async_connect for asynchronous connection
        net::async_connect(
            beast::get_lowest_layer(ws_), // Get the underlying TCP socket
            results,                      // The resolver results
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

        // Set SNI for SSL handshake (Host for WebSocket)
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
            // More detailed SSL error information
            char buf[256];
            ERR_error_string_n(ERR_get_error(), buf, sizeof(buf)); // Correct way to get SSL error from stack
            std::cerr << "[SSL DETAILS] OpenSSL Error: " << buf << std::endl;
            return handle_error(ec, "ssl_handshake");
        }

        std::cout << "[SECURE] SSL handshake successful" << std::endl;

        // Set suggested timeout settings for the websocket
        ws_.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client));

        // Perform WebSocket handshake using the dynamic path
        ws_.async_handshake(
            "stream.binance.com", // WebSocket host again
            websocket_url_path_,  // Use the constructed path
            beast::bind_front_handler(
                &Impl::on_handshake,
                this));
    }

    void on_handshake(beast::error_code ec) {
        if (ec) {
            std::cerr << "[HANDSHAKE ERROR] " << ec.message() << std::endl;
            std::cerr << "[HANDSHAKE DETAILS] Error code: " << ec.value()
                      << ", Category: " << ec.category().name() << std::endl;
            return handle_error(ec, "handshake");
        }

        std::cout << "[CONNECTED] WebSocket handshake successful to " << websocket_url_path_ << "!" << std::endl;
        // Update MarketData connected status
        data_.update_market_data(0.0, 0.0, 0, true, {}); // Set connected to true
        do_read(); // Start reading messages
    }

    void do_read() {
        // Clear the buffer before reading
        buffer_.consume(buffer_.size());
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
            buffer_.consume(buffer_.size()); // Consume the data after parsing

            // Binance depth updates ("diff.depth" stream) provide 'b' and 'a' for bids and asks.
            // Check the specific event type if you're subscribed to multiple streams.
            // For @depth20@100ms, the structure is usually 'bids' and 'asks' arrays.

            if (data.contains("bids") && data.contains("asks")) {
                // Ensure data["bids"] and data["asks"] are not empty to prevent out-of-bounds access
                double current_bid = 0.0;
                double current_ask = 0.0;

                if (!data["bids"].empty() && data["bids"][0].is_array() && data["bids"][0].size() >= 2) {
                    current_bid = std::stod(data["bids"][0][0].get<std::string>());
                }
                if (!data["asks"].empty() && data["asks"][0].is_array() && data["asks"][0].size() >= 2) {
                    current_ask = std::stod(data["asks"][0][0].get<std::string>());
                }


                uint64_t current_timestamp = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                    .count());

                std::vector<OrderBook::Order> recent_depth_updates;
                // Add bids
                if (data.contains("bids") && data["bids"].is_array()) {
                    for (const auto& bid_level : data["bids"]) {
                        if (bid_level.is_array() && bid_level.size() >= 2) {
                            recent_depth_updates.emplace_back(
                                std::stod(bid_level[0].get<std::string>()),
                                std::stod(bid_level[1].get<std::string>()),
                                true); // is_bid = true
                        }
                    }
                }
                // Add asks
                if (data.contains("asks") && data["asks"].is_array()) {
                    for (const auto& ask_level : data["asks"]) {
                        if (ask_level.is_array() && ask_level.size() >= 2) {
                            recent_depth_updates.emplace_back(
                                std::stod(ask_level[0].get<std::string>()),
                                std::stod(ask_level[1].get<std::string>()),
                                false); // is_bid = false
                        }
                    }
                }

                // Use the thread-safe update_market_data method
                data_.update_market_data(current_bid, current_ask, current_timestamp, true, recent_depth_updates);

            } else {
                // Handle other messages if necessary (e.g., subscription acknowledgements, pings)
                // std::cout << "[WS MESSAGE] Unhandled: " << data.dump() << std::endl;
            }

        } catch (const std::exception& e) {
            std::cerr << "[PARSE ERROR] " << e.what() << std::endl;
            std::cerr << "[RAW DATA] " << beast::buffers_to_string(buffer_.data()) << std::endl;
            buffer_.consume(buffer_.size()); // Try to clear buffer even on parse error
        }

        // Continue reading for the next message
        do_read();
    }

    void handle_error(beast::error_code ec, const char* what) {
        std::cerr << "[ERROR] " << what << " failed: "
                  << ec.message() << " (code: " << ec.value()
                  << ", category: " << ec.category().name() << ")" << std::endl;

        // Update MarketData connected status to false on error
        data_.update_market_data(0.0, 0.0, 0, false, {}); // Disconnected state
    }
};

// --- BinanceWSClient Public Interface Implementations ---

BinanceWSClient::BinanceWSClient(net::io_context& ioc, ssl::context& ctx,
                                 const std::string& symbol, int depth_level, const std::string& update_speed)
    : pimpl_(std::make_unique<Impl>(ioc, ctx, symbol, depth_level, update_speed)) {
    // Constructor delegates directly to PIMPL's constructor
    std::cout << "[BinanceWSClient] Constructed with symbol: " << symbol << std::endl;
}

BinanceWSClient::~BinanceWSClient() {
    std::cout << "[DESTRUCTOR] Cleaning up BinanceWSClient..." << std::endl;
    if (pimpl_) {
        // Explicitly call stop before unique_ptr destroys Impl
        pimpl_->stop();
    }
}

void BinanceWSClient::start() {
    pimpl_->run();
}

void BinanceWSClient::stop() {
    pimpl_->stop();
}

MarketData& BinanceWSClient::get_market_data() noexcept {
    return pimpl_->get_data();
}

// Synchronous get_snapshot implementation
std::vector<OrderBook::Order> BinanceWSClient::get_snapshot(int depth) {
    // This call delegates to the Impl's synchronous snapshot method.
    return pimpl_->get_snapshot_sync(depth);
}