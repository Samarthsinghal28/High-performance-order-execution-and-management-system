#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <mutex>

class WebSocketServer : public std::enable_shared_from_this<WebSocketServer> {
public:
    explicit WebSocketServer(unsigned short port, const std::string& token);

    void run();
    void subscribe(const std::string& symbol, std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws);
    void unsubscribe(const std::string& symbol, std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws);
    void unsubscribeAll(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws);
    void broadcastOrderBookUpdates(const std::string& symbol, const std::string& orderBook);
    std::map<std::string, std::vector<std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>>> subscribers;
    std::mutex subscribersMutex; // Thread safety

private:
    void acceptConnection();
    void handleConnection(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws);

    unsigned short port;
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::acceptor acceptor;
    std::string accessToken; // Add the access token

    
};

#endif // SERVER_H
