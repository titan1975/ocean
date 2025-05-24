#ifndef MARKETDATA_HPP
#define MARKETDATA_HPP

#include <string>
#include <mutex>   // For std::mutex
#include <cstdint> // For uint64_t
#include <vector>  // For std::vector

// Include your actual OrderBook header to use OrderBook::Order
// Ensure this path is correct relative to where MarketData.hpp is located.
#include "Core/OrderBook.hpp"

// Define a simple struct to hold a snapshot of market data.
// This struct contains only data members and is trivially copyable.
// It's designed to be a value-type that can be safely copied between threads.
struct MarketDataSnapshot {
    double bid = 0.0;
    double ask = 0.0;
    uint64_t timestamp = 0;  // Timestamp (milliseconds since epoch)
    bool connected = false;  // Connection status at the time of the snapshot

    // Vector to hold recent depth updates (bids and asks from the WS stream).
    // A copy of this vector will be made when a snapshot is taken.
    std::vector<OrderBook::Order> recent_depth_updates;
};

// The MarketData class manages the current state of market data,
// ensuring thread-safe access and updates.
class MarketData {
public:
    // Default constructor. Member variables are default-initialized.
    MarketData() = default;

    // Public method to retrieve a consistent, thread-safe snapshot of the current market data.
    // This method is marked 'const' because it does not modify the state of the MarketData object.
    MarketDataSnapshot get_updates() const;

    // Public method to update the internal market data.
    // This method should be called by the WebSocket client (e.g., BinanceWSClient::Impl::on_read)
    // whenever new market data arrives. It handles locking internally.
    void update_market_data(
        double new_bid,
        double new_ask,
        uint64_t new_timestamp,
        bool new_connected,
        const std::vector<OrderBook::Order>& new_depth_updates
    );

private:
    // Mutex to protect access to the data members below.
    // This ensures that only one thread can read or write these members at a time.
    // The trailing underscore `_` is a common convention for private member variables.
    mutable std::mutex mutex_; // 'mutable' is needed because get_updates() is const

    // Private members to store the latest market data.
    // These are updated via the `update_market_data` method and read via `get_updates`.
    double bid_ = 0.0;        // Latest best bid price.
    double ask_ = 0.0;        // Latest best ask price.
    uint64_t timestamp_ = 0;  // Timestamp (milliseconds since epoch) of the last update.
    bool connected_ = false;  // Connection status of the WebSocket.

    // Vector to hold recent depth updates.
    std::vector<OrderBook::Order> recent_depth_updates_;
};

#endif // MARKETDATA_HPP