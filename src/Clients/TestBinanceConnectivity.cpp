#include <gtest/gtest.h>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

TEST(BinanceClient, BasicConnectivity) {
    try {
        // 1. Create core objects
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        
        // 2. Connect to Binance
        websocket::stream<tcp::socket> ws(ioc);
        auto const results = resolver.resolve("stream.binance.com", "9443");
        net::connect(ws.next_layer(), results.begin(), results.end());
        
        // 3. Perform handshake
        ws.handshake("stream.binance.com", "/ws/btcusdt@depth@100ms");
        
        // 4. Immediate close (we just want to test connectivity)
        ws.close(websocket::close_code::normal);
        
        SUCCEED();  // If we get here, connection worked
    } catch (const boost::system::system_error& e) {
        FAIL() << "Connection failed: " << e.what() 
               << " (code: " << e.code() << ")";
    }
}