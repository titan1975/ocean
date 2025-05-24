#include "Core/OrderBook.hpp"
#include <algorithm> // For std::find_if, std::sort (if you were sorting), etc.
#include <iostream>  // For std::cerr
#include <mutex>

void OrderBook::initialize(const std::vector<Order>& snapshot) noexcept {
    // Acquire a writer lock for exclusive access to the order book.
    std::unique_lock lock(mtx_);

    // Clear any existing data in the order book.
    bids_.clear();
    asks_.clear();

    // Process each order from the provided snapshot.
    for (const auto& order : snapshot) {
        // Determine whether it's a bid or an ask and get the corresponding book side.
        auto& side = order.is_bid ? bids_ : asks_;

        // Use try_emplace to insert a new price level or get an iterator to an existing one.
        // It's efficient as it avoids default construction followed by assignment.
        auto [it, inserted] = side.try_emplace(order.price, PriceLevel{});

        // Aggregate the amount for the current price level.
        it->second.total_amount += order.amount;
        it->second.order_count++;
    }

    // Optional: Log a warning if one of the book sides is empty after initialization.
    // This can indicate an issue with the snapshot data or an unexpected market state.
    if (bids_.empty() || asks_.empty()) {
        std::cerr << "Warning: Order book initialized with empty side(s)\n";
    }
}

void OrderBook::update(const std::vector<Order>& orders) noexcept {
    // Acquire a writer lock for exclusive access during the update process.
    std::unique_lock lock(mtx_);

    for (const auto& order : orders) {
        auto& side = order.is_bid ? bids_ : asks_;

        // Find the price level.
        auto it = side.find(order.price);

        if (it != side.end()) {
            // Price level exists.
            if (order.amount == static_cast<std::float32_t>(0.0)) {
                // If the update amount is 0, it signifies removal of the level.
                side.erase(it);
            } else {
                // Otherwise, update the amount to the new absolute value.
                it->second.total_amount = order.amount;
                // Note: order_count is not updated here. If it needs to reflect
                // individual orders, more complex logic is required.
            }
        } else {
            // Price level does not exist.
            if (order.amount > static_cast<std::float32_t>(0.0)) {
                // Insert a new level only if the amount is positive.
                side.emplace(order.price, PriceLevel{order.amount, 1});
            }
            // If price level doesn't exist and order.amount is 0 or negative, do nothing (effectively ignored).
        }
    }

    // After applying updates, prune any price levels that now have zero or negative total amount.
    // This loop acts as a safeguard for any unhandled negative amounts or edge cases.
    for (auto* side_ptr : {&bids_, &asks_}) { // Iterate over both bid and ask maps
        for (auto it = side_ptr->begin(); it != side_ptr->end(); ) {
            if (it->second.total_amount <= static_cast<std::float32_t>(0.0)) {
                // If amount is zero or less, erase the price level.
                it = side_ptr->erase(it);
            } else {
                // Otherwise, move to the next level.
                ++it;
            }
        }
    }
}

std::float32_t OrderBook::get_mid_price() const noexcept {
    // Acquire a reader lock for shared access to the order book.
    std::shared_lock lock(mtx_);

    // If either side is empty, a mid-price cannot be calculated.
    if (bids_.empty() || asks_.empty()) {
        return static_cast<std::float32_t>(0.0); // Return 0.0f or throw an exception, depending on your error handling policy.
    }

    // The best bid is the highest price in the bids_ map (last element for std::map).
    std::float32_t best_bid = bids_.rbegin()->first;
    // The best ask is the lowest price in the asks_ map (first element for std::map).
    std::float32_t best_ask = asks_.begin()->first;

    // Calculate and return the mid-price.
    return (best_bid + best_ask) / static_cast<std::float32_t>(2.0);
}

std::pair<std::float32_t, std::float32_t> OrderBook::get_bbo() const noexcept {
    // Acquire a reader lock for shared access.
    std::shared_lock lock(mtx_);

    // Get the highest bid price. If bids_ is empty, return 0.0f.
    const std::float32_t best_bid = bids_.empty() ? static_cast<std::float32_t>(0.0) : bids_.rbegin()->first;
    // Get the lowest ask price. If asks_ is empty, return 0.0f.
    const std::float32_t best_ask = asks_.empty() ? static_cast<std::float32_t>(0.0) : asks_.begin()->first;

    return {best_bid, best_ask};
}

std::float32_t OrderBook::total_bid_volume() const noexcept {
    // Acquire a reader lock for shared access.
    std::shared_lock lock(mtx_);
    std::float32_t total = static_cast<std::float32_t>(0.0);
    // Sum the total_amount from all bid price levels.
    for (const auto& pair : bids_) {
        total += pair.second.total_amount;
    }
    return total;
}

std::float32_t OrderBook::total_ask_volume() const noexcept {
    // Acquire a reader lock for shared access.
    std::shared_lock lock(mtx_);
    std::float32_t total = static_cast<std::float32_t>(0.0);
    // Sum the total_amount from all ask price levels.
    for (const auto& pair : asks_) {
        total += pair.second.total_amount;
    }
    return total;
}
