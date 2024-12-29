#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <atomic>
#include <condition_variable>
#include "./functions/functions.cpp"

using json = nlohmann::json;

class TradingSystem {
private:
    std::atomic<bool> running{true};
    std::mutex mutex;
    std::condition_variable cv;
    WebSocketServer& server;
    std::string accessToken;

public:
    TradingSystem(WebSocketServer& srv, const std::string& token)
        : server(srv), accessToken(token) {}

    bool isRunning() const {
        return running.load();
    }

    void stop() {
        running.store(false);
        cv.notify_all();
    }

    void handleUserCommands() {
        while (isRunning()) {
            try {
                std::cout << "\n--- User Command Menu ---\n"
                         << "1. Place Order\n"
                         << "2. Cancel Order\n"
                         << "3. Modify Order\n"
                         << "4. Get Open Orders\n"
                         << "5. View Current Position\n"
                         << "6. Quit\n"
                         << "Enter your choice: ";

                int choice;
                if (!(std::cin >> choice)) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cout << "Invalid input. Please enter a number.\n";
                    continue;
                }

                switch (choice) {
                    case 1: {
                        std::string type, instrument;
                        double price, amount;
                        std::cout << "Enter order details (type price amount instrument): ";
                        if (std::cin >> type >> price >> amount >> instrument) {
                            std::string response = placeOrder(type, std::to_string(price), 
                                                           accessToken, std::to_string(amount), 
                                                           instrument);
                            std::cout << "Order Response: " << response << "\n";
                        }
                        break;
                    }
                    case 2: {
                        std::string orderId;
                        std::cout << "Enter Order ID to cancel: ";
                        if (std::cin >> orderId) {
                            std::string response = cancelOrder(orderId, accessToken);
                            std::cout << "Cancel Response: " << response << "\n";
                        }
                        break;
                    }
                    case 3: {
                        std::string orderId;
                        double newPrice;
                        std::cout << "Enter Order ID and new price: ";
                        if (std::cin >> orderId >> newPrice) {
                            std::string response = modifyOrder(orderId, std::to_string(newPrice), 
                                                            accessToken, newPrice);
                            std::cout << "Modify Response: " << response << "\n";
                        }
                        break;
                    }
                    case 4: {
                        std::string response = getOpenOrders(accessToken);
                        std::cout << "Open Orders:\n" << response << "\n";
                        break;
                    }
                    case 5: {
                        std::string instrument;
                        std::cout << "Enter instrument name: ";
                        if (std::cin >> instrument) {
                            std::string response = viewCurrentPosition(accessToken, instrument);
                            std::cout << "Position:\n" << response << "\n";
                        }
                        break;
                    }
                    case 6: {
                        exit(1);
                        return;
                    }
                    default: {
                        std::cout << "Invalid choice.\n";
                        break;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
            }
        }
    }

    void handleSubscriptionUpdates() {
        while (isRunning()) {
            try {
                std::unique_lock<std::mutex> lock(mutex);
                if (cv.wait_for(lock, std::chrono::seconds(1), 
                              [this] { return !isRunning(); })) {
                    return;
                }
                fetchOrderBookUpdates(server, accessToken);
            } catch (const std::exception& e) {
                std::cerr << "Subscription error: " << e.what() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
};

int main() {
    try {
        // Initialize CURL globally
        curl_global_init(CURL_GLOBAL_ALL);

        const std::string clientId = "cVB4mBlG";
        const std::string clientSecret = "XInK6DfGVTFKTbM1u46zRXiCL6NZSfWY98ilIzi985M";

        std::string accessToken = getAccessToken(clientId, clientSecret);
        if (accessToken.empty()) {
            throw std::runtime_error("Failed to get access token");
        }

        // Initialize WebSocket server
        unsigned short port = 8080;
        WebSocketServer server(port, accessToken);
        
        // Create trading system
        TradingSystem tradingSystem(server, accessToken);

        // Start server thread
        std::thread serverThread([&server]() {
            try {
                server.run();
            } catch (const std::exception& e) {
                std::cerr << "Server error: " << e.what() << "\n";
            }
        });

        // Start subscription thread
        std::thread subscriptionThread([&tradingSystem]() {
            tradingSystem.handleSubscriptionUpdates();
        });

        // Delay to allow the server run properly.
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Handle user commands in main thread
        tradingSystem.handleUserCommands();

        // Cleanup
        tradingSystem.stop();
        
        if (serverThread.joinable()) {
            serverThread.join();
        }
        if (subscriptionThread.joinable()) {
            subscriptionThread.join();
        }

        // Cleanup CURL
        curl_global_cleanup();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}