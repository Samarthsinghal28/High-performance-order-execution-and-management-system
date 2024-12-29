#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

class WebSocketClient {
private:
    net::io_context& ioc;
    websocket::stream<tcp::socket> ws;
    std::string host;
    std::string port;
    bool connected;
    mutable std::mutex mtx;  // Made mutable for const member functions
    std::condition_variable cv;
    bool should_stop{false};

public:
    WebSocketClient(net::io_context& ioc, const std::string& host, const std::string& port)
        : ioc(ioc)
        , ws(ioc)
        , host(host)
        , port(port)
        , connected(false) {
    }

    void connect() {
        try {
            tcp::resolver resolver{ioc};
            auto const results = resolver.resolve(host, port);
            auto ep = net::connect(ws.next_layer(), results);
            
            std::string host_str = host + ':' + std::to_string(ep.port());

            ws.set_option(websocket::stream_base::decorator(
                [](websocket::request_type& req) {
                    req.set(http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-client-async");
                }));

            ws.handshake(host_str, "/");
            
            {
                std::lock_guard<std::mutex> lock(mtx);
                connected = true;
            }
            cv.notify_all();
            std::cout << "Connected to WebSocket server" << std::endl;

        } catch(std::exception const& e) {
            std::cerr << "Connect error: " << e.what() << std::endl;
            throw;
        }
    }

    void subscribe(const std::string& symbol) {
        try {
            std::string msg = "subscribe:" + symbol;
            ws.write(net::buffer(msg));
            std::cout << "Subscription request sent for: " << symbol << std::endl;
        } catch(std::exception const& e) {
            std::cerr << "Subscribe error: " << e.what() << std::endl;
            throw;
        }
    }

    void readMessages() {
        try {
            beast::flat_buffer buffer;
            
            while(!should_stop) {
                ws.read(buffer);
                
                // Get received timestamp
                auto timeReceived = std::chrono::system_clock::now().time_since_epoch().count();
                
                // Convert buffer to string
                std::string msg = beast::buffers_to_string(buffer.data());
                
                try {
                    json payload = json::parse(msg);
                    
                    // Extract timestamp and message if available
                    if (payload.contains("timestamp") && payload.contains("message")) {
                        auto timeSent = payload["timestamp"].get<int64_t>();
                        auto message = payload["message"].get<std::string>();
                        auto propagationDelay = timeReceived - timeSent;
                        
                        std::cout << "Message: " << message << std::endl;
                        std::cout << "Propagation Delay: " << propagationDelay << " ns" << std::endl;
                    } else {
                        // Handle raw message
                        std::cout << "Received: " << msg << std::endl;
                    }
                } catch (const json::parse_error& e) {
                    // Handle non-JSON messages
                    std::cout << "Received (raw): " << msg << std::endl;
                }
                
                buffer.consume(buffer.size());
            }
        } catch(std::exception const& e) {
            if (!should_stop) {
                std::cerr << "Read error: " << e.what() << std::endl;
                throw;
            }
        }
    }

    void close() {
        try {
            should_stop = true;
            ws.close(websocket::close_code::normal);
            std::cout << "Connection closed" << std::endl;
        } catch(std::exception const& e) {
            std::cerr << "Close error: " << e.what() << std::endl;
            throw;
        }
    }

    bool isConnected() const {
        std::lock_guard<std::mutex> lock(mtx);
        return connected;
    }

    void waitForConnection() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return connected; });
    }
};

int main() {
    try {
        net::io_context ioc;
        WebSocketClient client(ioc, "127.0.0.1", "8080");

        std::thread connect_thread([&client]() {
            try {
                client.connect();
            } catch(std::exception const& e) {
                std::cerr << "Connection thread error: " << e.what() << std::endl;
            }
        });

        client.waitForConnection();

        if (!client.isConnected()) {
            std::cerr << "Failed to connect" << std::endl;
            return EXIT_FAILURE;
        }

        client.subscribe("BTC-PERPETUAL");

        std::thread read_thread([&client]() {
            try {
                client.readMessages();
            } catch(std::exception const& e) {
                std::cerr << "Read thread error: " << e.what() << std::endl;
            }
        });

        std::cout << "Enter 'quit' to exit" << std::endl;
        std::string input;
        while(std::getline(std::cin, input)) {
            if(input == "quit") {
                break;
            }
        }

        client.close();
        
        if(connect_thread.joinable()) {
            connect_thread.join();
        }
        if(read_thread.joinable()) {
            read_thread.join();
        }

        return EXIT_SUCCESS;

    } catch(std::exception const& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}