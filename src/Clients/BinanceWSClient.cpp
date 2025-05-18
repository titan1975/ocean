#include <iostream>
#include <string>
#include <mutex>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/strand.hpp>
#include <thread>

#include <boost/asio/ip/tcp.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

struct OrderBook {
    std::mutex mutex;
    double bid = 0, ask = 0;

    void update(const json& data) {
        std::lock_guard<std::mutex> lock(mutex);
        bid = std::stod(data["bids"][0][0].get<std::string>());
        ask = std::stod(data["asks"][0][0].get<std::string>());
    }
};

class BinanceWSClient {
    boost::beast::flat_buffer buffer_;
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<tcp::socket>> ws_;
    OrderBook& book_;
    std::string symbol_;

    public:
    BinanceWSClient(net::io_context& ioc, ssl::context& ctx, OrderBook& book, const std::string& symbol)
       : resolver_(net::make_strand(ioc))
       , ws_(net::make_strand(ioc), ctx)
       , book_(book)
       , symbol_(symbol)
    {}

    void run() {
        resolver_.async_resolve("stream.binance.com", "9443",
            beast::bind_front_handler(&BinanceWSClient::on_resolve, this));
    }

    void stop() {
        ws_.async_close(websocket::close_code::normal,
            [](beast::error_code ec){});
    }

private:


    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) return fail(ec, "resolve");

        boost::asio::async_connect(
     beast::get_lowest_layer(ws_),
     results.begin(), results.end(),
     beast::bind_front_handler(&BinanceWSClient::on_connect, this));

    }

     void on_connect(beast::error_code ec, tcp::endpoint ep) {
        if (ec) return fail(ec, "connect");

        // Set SNI hostname
        SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), "stream.binance.com");

        ws_.next_layer().async_handshake(ssl::stream_base::client,
            beast::bind_front_handler(&BinanceWSClient::on_ssl_handshake, this));
    }

    void on_ssl_handshake(beast::error_code ec) {
        if (ec) return fail(ec, "ssl_handshake");

        ws_.async_handshake("stream.binance.com", "/ws/" + symbol_ + "@depth20@100ms",
            beast::bind_front_handler(&BinanceWSClient::on_handshake, this));
    }

    void on_handshake(beast::error_code ec) {
        if (ec) return fail(ec, "ws_handshake");
        do_read();
    }

public:
    void do_read() {
        ws_.async_read(buffer_,
            beast::bind_front_handler(&BinanceWSClient::on_read, this));
    }

    void on_read(beast::error_code ec, size_t) {
        if (ec) return fail(ec, "read");

        try {
            book_.update(json::parse(beast::buffers_to_string(buffer_.data())));
            buffer_.consume(buffer_.size());

            std::lock_guard<std::mutex> lock(book_.mutex);
            std::cout << symbol_ << " | Bid: " << book_.bid
                      << " | Ask: " << book_.ask << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "JSON Error: " << e.what() << std::endl;
        }

        do_read();
    }

    void fail(beast::error_code ec, const char* what) {
        std::cerr << what << ": " << ec.message() << std::endl;
    }
};

int main() {
    net::io_context ioc;
    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();

    OrderBook btc_book, eth_book;

    BinanceWSClient btc_client(ioc, ctx, btc_book, "btcusdt");
    BinanceWSClient eth_client(ioc, ctx, eth_book, "ethusdt");

    btc_client.run();
    eth_client.run();

    std::thread t([&ioc]{ ioc.run(); });

    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.ignore();

    btc_client.stop();
    eth_client.stop();
    t.join();

    return 0;
}