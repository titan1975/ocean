#include "Core/MarketData.hpp" // Correctly include its own header here

// Implementation of the get_updates method.
// This method provides a consistent snapshot of the market data.
MarketDataSnapshot MarketData::get_updates() const {
    // Acquire a lock on the mutex_.
    // std::lock_guard ensures the mutex is automatically released when 'lock' goes out of scope,
    // even if an exception occurs. This guarantees thread safety for reading the data.
    std::lock_guard<std::mutex> lock(mutex_);

    // Create a MarketDataSnapshot object.
    MarketDataSnapshot snapshot;

    // Copy the current values of the protected member variables into the snapshot.
    snapshot.bid = bid_;
    snapshot.ask = ask_;
    snapshot.timestamp = timestamp_;
    snapshot.connected = connected_;
    snapshot.recent_depth_updates = recent_depth_updates_; // Performs a copy of the vector

    // Return the snapshot. The snapshot is a value-type and can be safely used by other threads.
    return snapshot;
}

// Implementation of the update_market_data method.
// This method updates the internal state of the MarketData object in a thread-safe manner.
void MarketData::update_market_data(
    double new_bid,
    double new_ask,
    uint64_t new_timestamp,
    bool new_connected,
    const std::vector<OrderBook::Order>& new_depth_updates
) {
    // Acquire a lock on the mutex_ before modifying any member variables.
    // This ensures that no other thread can read or write while an update is in progress.
    std::lock_guard<std::mutex> lock(mutex_);

    // Update the member variables with the new data.
    bid_ = new_bid;
    ask_ = new_ask;
    timestamp_ = new_timestamp;
    connected_ = new_connected;
    recent_depth_updates_ = new_depth_updates; // Assigns the new vector, potentially copying data
}