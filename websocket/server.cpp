#include "server.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include "../functions/functions.h"
using namespace boost::asio;
using namespace boost::beast;

//helper functions
std::vector<std::string> splitCommand(const std::string& command, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(command);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}


void sendError(std::shared_ptr<websocket::stream<ip::tcp::socket>> ws, const std::string& errorMessage) {
    ws->async_write(net::buffer("Error: " + errorMessage), [](boost::system::error_code writeEc, std::size_t) {
        if (writeEc) {
            std::cerr << "Write error: " << writeEc.message() << std::endl;
        }
    });
}


WebSocketServer::WebSocketServer(unsigned short port, const std::string& token)
    : port(port), acceptor(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)), accessToken(token) {}


void WebSocketServer::run() {
    std::cout << "WebSocket server is running on port " << port << std::endl;
    acceptConnection();
    ioContext.run();
}

void WebSocketServer::acceptConnection() {
    auto socket = std::make_shared<ip::tcp::socket>(ioContext);
    acceptor.async_accept(*socket, [this, socket](boost::system::error_code ec) {
    if (!ec) {
        auto ws = std::make_shared<websocket::stream<ip::tcp::socket>>(std::move(*socket));
        ws->async_accept([this, ws](boost::system::error_code ec) {
            if (!ec) {
                handleConnection(ws);
            } else {
                std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
            }
        });
    } else {
        std::cerr << "TCP accept error: " << ec.message() << std::endl;
    }
    acceptConnection(); // Continue accepting connections
});
}

void WebSocketServer::handleConnection(std::shared_ptr<websocket::stream<ip::tcp::socket>> ws) {
    auto buffer = std::make_shared<flat_buffer>();

    ws->async_read(*buffer, [this, ws, buffer](boost::system::error_code ec, std::size_t bytesTransferred) {
        if (!ec) {
            try {
                std::string message = buffers_to_string(buffer->data());
                // std::cout << "Received: " << message << std::endl;

                if (message.find("place_order:") == 0) {
                    // Place order: place_order:<type>:<price>:<amount>:<instrument>
                    auto args = splitCommand(message, ':');
                    if (args.size() == 5) {
                        std::string response = placeOrder(args[1], args[2], accessToken, args[3], args[4]);
                        ws->async_write(net::buffer(response), [](boost::system::error_code writeEc, std::size_t) {
                            if (writeEc) {
                                std::cerr << "Write error: " << writeEc.message() << std::endl;
                            }
                        });
                    } else {
                        sendError(ws, "Invalid place_order command format.");
                    }
                } else if (message.find("cancel:") == 0) {
                    // Cancel order: cancel:<order_id>
                    std::string orderId = message.substr(7);
                    std::string response = cancelOrder(orderId, accessToken);
                    ws->async_write(net::buffer(response), [](boost::system::error_code writeEc, std::size_t) {
                        if (writeEc) {
                            std::cerr << "Write error: " << writeEc.message() << std::endl;
                        }
                    });
                } else if (message.find("modify:") == 0) {
                    // Modify order: modify:<order_id>:<new_price>:<new_amount>
                    auto args = splitCommand(message, ':');
                    if (args.size() == 4) {
                        std::string response = modifyOrder(args[1], args[2], accessToken, std::stoi(args[3]));
                        ws->async_write(net::buffer(response), [](boost::system::error_code writeEc, std::size_t) {
                            if (writeEc) {
                                std::cerr << "Write error: " << writeEc.message() << std::endl;
                            }
                        });
                    } else {
                        sendError(ws, "Invalid modify command format.");
                    }
                } else if (message.find("get_orderbook:") == 0) {
                    // Get orderbook: get_orderbook:<instrument>
                    std::string instrument = message.substr(14);
                    std::string response = getOrderBook(instrument, accessToken);
                    ws->async_write(net::buffer(response), [](boost::system::error_code writeEc, std::size_t) {
                        if (writeEc) {
                            std::cerr << "Write error: " << writeEc.message() << std::endl;
                        }
                    });
                } else if (message.find("view_positions:") == 0) {
                    // View positions: view_positions:<instrument>
                    std::string instrument = message.substr(15);
                    std::string response = viewCurrentPosition(accessToken, instrument);
                    ws->async_write(net::buffer(response), [](boost::system::error_code writeEc, std::size_t) {
                        if (writeEc) {
                            std::cerr << "Write error: " << writeEc.message() << std::endl;
                        }
                    });
                } else if (message.find("subscribe:") == 0) {
                    // Subscribe to symbol
                    std::string symbol = message.substr(10);
                    subscribe(symbol, ws);
                    ws->async_write(net::buffer("Subscribed to: " + symbol), [](boost::system::error_code writeEc, std::size_t) {
                        if (writeEc) {
                            std::cerr << "Write error: " << writeEc.message() << std::endl;
                        }
                    });
                } else if (message.find("unsubscribe:") == 0) {
                    // Unsubscribe from symbol
                    std::string symbol = message.substr(12);
                    unsubscribe(symbol, ws);
                    ws->async_write(net::buffer("Unsubscribed from: " + symbol), [](boost::system::error_code writeEc, std::size_t) {
                        if (writeEc) {
                            std::cerr << "Write error: " << writeEc.message() << std::endl;
                        }
                    });
                } else {
                    sendError(ws, "Unrecognized command.");
                }

                // Continue reading
                handleConnection(ws);
            } catch (const std::exception& e) {
                std::cerr << "Exception in handleConnection: " << e.what() << std::endl;
                ws->close(websocket::close_code::internal_error);
            }
        } else if (ec == websocket::error::closed) {
            std::cout << "Client closed the connection. Cleaning up resources." << std::endl;
            unsubscribeAll(ws); // Implement a function to remove client subscriptions
        } else if (ec == boost::asio::error::operation_aborted) {
            std::cout << "Read operation canceled. Client disconnected." << std::endl;
        } else {
            std::cerr << "Unexpected error: " << ec.message() << std::endl;
        }
    });
}


void WebSocketServer::unsubscribeAll(std::shared_ptr<websocket::stream<ip::tcp::socket>> ws) {
    std::lock_guard<std::mutex> lock(subscribersMutex);

    for (auto it = subscribers.begin(); it != subscribers.end();) {
        auto& clientList = it->second;
        clientList.erase(std::remove(clientList.begin(), clientList.end(), ws), clientList.end());
        if (clientList.empty()) {
            it = subscribers.erase(it); // Remove empty subscription lists
        } else {
            ++it;
        }
    }
    std::cout << "Cleaned up subscriptions for disconnected client." << std::endl;
}




void WebSocketServer::subscribe(const std::string& symbol, std::shared_ptr<websocket::stream<ip::tcp::socket>> ws) {
    std::lock_guard<std::mutex> lock(subscribersMutex);
    subscribers[symbol].push_back(ws);
    // std::cout << "Subscribed to: " << symbol << std::endl;
}

void WebSocketServer::unsubscribe(const std::string& symbol, std::shared_ptr<websocket::stream<ip::tcp::socket>> ws) {
    std::lock_guard<std::mutex> lock(subscribersMutex);
    auto& vec = subscribers[symbol];
    vec.erase(std::remove(vec.begin(), vec.end(), ws), vec.end());
    if (vec.empty()) {
        subscribers.erase(symbol);
    }
}

void WebSocketServer::broadcastOrderBookUpdates(const std::string& symbol, const std::string& orderBook) {
    std::lock_guard<std::mutex> lock(subscribersMutex);

    auto timeSent = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    json payload;
    payload["timestamp"] = timeSent;
    payload["message"] = orderBook;

    std::string serializedPayload = payload.dump();

    
    auto it = subscribers.find(symbol);
 
    if (it != subscribers.end()) {
        for (auto& ws : it->second) {
            if (ws && ws->is_open()) {
                try {
                    ws->write(net::buffer(serializedPayload));
                } catch (const std::exception& e) {
                    std::cerr << "Broadcast error: " << e.what() << std::endl;
                }
            }
        }
    }
}

