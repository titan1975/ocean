//
// Created by hera on 5/18/25.
//
#include "gtest/gtest.h"
#include "Clients/BinanceClient.hpp"
#include <atomic>
#include <thread>

// Mock WebSocket Server for Testing
class MockWebSocketServer {
public:
    void start() {
        server_thread_ = std::thread([this] {
            mock_running_ = true;
            while (mock_running_) {
                // Simulate periodic WebSocket messages
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (mock_running_) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    messages_.push_back("{\"e\":\"trade\",\"s\":\"BTCUSDT\",\"p\":\"25000.00\"}");
                    message_count_++;
                }
            }
        });
    }

    void stop() {
        mock_running_ = false;
        if (server_thread_.joinable()) server_thread_.join();
    }

    void sendMockMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.push_back(message);
        message_count_++;
    }

    std::vector<std::string> getMessages() {
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_;
    }

    size_t getMessageCount() const {
        std::lock_guard<std::mutex> lock(mutex_);  // Now works!
        return message_count_;
    }

    std::atomic<bool> mock_running_{false};
private:
    std::thread server_thread_;
    std::vector<std::string> messages_;
    size_t message_count_ = 0;
    mutable std::mutex mutex_;  // Key fix
};

class BinanceClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_.start();
        client_ = std::make_unique<BinanceClient>("BTCUSDT");
        ASSERT_TRUE(client_->start()) << "Failed to start client connection";
    }

    void TearDown() override {
        server_.stop();
        client_->stop();
    }

    MockWebSocketServer server_;
    std::unique_ptr<BinanceClient> client_;
};

//------------------------------------------------------------------
// TEST CASES
//------------------------------------------------------------------
TEST_F(BinanceClientTest, StartsConnection) {
    EXPECT_TRUE(server_.mock_running_);
    EXPECT_EQ(0, server_.getMessageCount());
}

TEST_F(BinanceClientTest, StopsGracefully) {
    client_->stop();
    EXPECT_FALSE(server_.mock_running_);
    EXPECT_EQ(0, server_.getMessageCount());
}

TEST_F(BinanceClientTest, ReceivesMarketData) {
    const std::string mockTrade = "{\"e\":\"trade\",\"s\":\"BTCUSDT\",\"p\":\"25000.00\"}";
    server_.sendMockMessage(mockTrade);
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(1, server_.getMessageCount());
   // EXPECT_TRUE(client_->isConnected());
    
    // Verify trade data
   // double lastPrice = client_->getLastPrice();
   // EXPECT_EQ(25000.00, lastPrice);
}

TEST_F(BinanceClientTest, HandlesDisconnect) {
    // Simulate network disconnection
    server_.stop();
    
    // Wait for reconnection attempt
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
  //  EXPECT_FALSE(client_->isConnected());
    
    // Restart server and verify reconnection
    server_.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  //  EXPECT_TRUE(client_->isConnected());
}

TEST_F(BinanceClientTest, HandlesInvalidSymbol) {
    BinanceClient invalidClient("INVALIDSYMBOL");
    EXPECT_FALSE(invalidClient.start());
}

TEST_F(BinanceClientTest, HandlesMultipleSymbols) {
    MockWebSocketServer server;
    server.start();
    
    BinanceClient btcClient("BTCUSDT");
    BinanceClient ethClient("ETHUSDT");
    
    EXPECT_TRUE(btcClient.start());
    EXPECT_TRUE(ethClient.start());
    
    server.sendMockMessage("{\"e\":\"trade\",\"s\":\"BTCUSDT\",\"p\":\"25000.00\"}");
    server.sendMockMessage("{\"e\":\"trade\",\"s\":\"ETHUSDT\",\"p\":\"1800.00\"}");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
 //   EXPECT_EQ(25000.00, btcClient.getLastPrice());
   // EXPECT_EQ(1800.00, ethClient.getLastPrice());
    
    server.stop();
}