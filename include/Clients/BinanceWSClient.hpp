#ifndef BINANCEWSCLIENT_HPP
#define BINANCEWSCLIENT_HPP

#include <memory>     // For std::unique_ptr
#include <string>
#include <vector>
#include <iostream>   // For debugging output in constructor

// Boost.Asio for networking
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

// Include the MarketData class definition
#include "Core/MarketData.hpp"
#include "Core/OrderBook.hpp"

// Forward declare to avoid circular dependencies if MarketData needs BinanceWSClient.
// Although in our current structure, MarketData does not depend on BinanceWSClient.
// This is typically only needed if MarketData had a pointer/reference to BinanceWSClient.
// class MarketData; // Not strictly necessary here as MarketData.hpp is included.

namespace net = boost::asio;
namespace ssl = net::ssl;

class BinanceWSClient {
public:
    // Constructor: Takes an io_context, SSL context, the trading symbol,
    // desired depth level for WS stream, and update speed.
    // The main function provides these parameters in its constructor call.
    BinanceWSClient(net::io_context& ioc, ssl::context& ctx,
                    const std::string& symbol,
                    int depth_level = 20, // Default to depth 20
                    const std::string& update_speed = "100ms"); // Default to 100ms

    // Destructor: Ensures the WebSocket connection is properly closed.
    ~BinanceWSClient();

    // Starts the WebSocket connection and begins streaming data.
    void start();

    // Stops the WebSocket connection.
    void stop();

    // Returns a reference to the internal MarketData object, providing access
    // to the latest bid, ask, and connection status.
    MarketData& get_market_data() noexcept;

    // Fetches an order book snapshot via Binance's REST API.
    // The 'depth' parameter specifies how many levels of the order book to retrieve.
    // This method is synchronous (blocking) and needs to be implemented.
    std::vector<OrderBook::Order> get_snapshot(int depth);

private:
    // PIMPL (Pointer to IMPLementation) idiom to hide internal details and dependencies.
    // This minimizes compilation dependencies on boost::beast details for users of BinanceWSClient.
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    // No need for a separate symbol_ here, it's managed within Impl.
};

#endif // BINANCEWSCLIENT_HPP