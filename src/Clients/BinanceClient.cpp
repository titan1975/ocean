//
// Created by hera on 5/17/25.
//
#include "Clients/BinanceClient.hpp"
#include <iostream>

#include "Core/OrderBook.hpp"

BinanceClient::BinanceClient(std::string symbol)
    : symbol_(std::move(symbol)) {}

bool BinanceClient::start() noexcept {
    running_ = true;
    std::cout << "Connecting to Binance " << symbol_ << "...\n";
    return true; // TODO: Implement WebSocket
}

void BinanceClient::stop() noexcept {
    running_ = false;
}

std::span<const OrderBook::Order> BinanceClient::get_updates() noexcept {
    static OrderBook::Order dummy_orders[1];
    return dummy_orders; // TODO: Real data
}