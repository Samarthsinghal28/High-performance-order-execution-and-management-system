#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include "../websocket/server.cpp"

using json = nlohmann::json;

// Function to handle the response from the cURL request
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// General function to send a cURL request with optional access token
std::string sendRequest(const std::string& url, const json& payload, const std::string& accessToken) {
    std::string readBuffer;

    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Set the HTTP method to POST
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Convert payload to string and set it as the POST fields
        std::string jsonStr = payload.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());

        // Set headers
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!accessToken.empty()) {
            std::string authHeader = "Authorization: Bearer " + accessToken;
            headers = curl_slist_append(headers, authHeader.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set SSL verification to bypass for debugging
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Set up the write callback to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "cURL error: %s\n", curl_easy_strerror(res));
        }

        // Print raw response for debugging
        // std::cout << "Raw Response: " << readBuffer << std::endl;

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();

    return readBuffer;
}

// Function to get the access token
std::string getAccessToken(const std::string& clientId, const std::string& clientSecret) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 0},
        {"method", "public/auth"},
        {"params", {
            {"grant_type", "client_credentials"},
            {"client_id", clientId},
            {"client_secret", clientSecret}
        }}
    };

    std::string response = sendRequest("https://test.deribit.com/api/v2/public/auth", payload);
    if (response.empty()) {
        std::cerr << "Failed to receive a response or response is empty." << std::endl;
        return "Failed to receive a response or response is empty.";
    }

    auto responseJson = json::parse(response, nullptr, false);
    if (responseJson.is_discarded()) {
        std::cerr << "Failed to parse JSON response." << std::endl;
        return "Failed to parse JSON response.";
    }

    // Retrieve the access token from the response
    if (responseJson.contains("result") && responseJson["result"].contains("access_token")) {
        return responseJson["result"]["access_token"];
    } else {
        std::cerr << "Failed to retrieve access token." << std::endl;
        return "";
    }
}

// Function to place an order
std::string placeOrder(const std::string& orderType, const std::string& price, const std::string& accessToken, const std::string& amount, const std::string& instrument) {
    
    auto startTime = std::chrono::high_resolution_clock::now();


    if (orderType != "buy" && orderType != "sell") {
        std::cerr << "Invalid order type. Must be 'buy' or 'sell'." << std::endl;
        return "Invalid order type. Must be 'buy' or 'sell'.";
    }

    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", std::string("private/") + orderType},
        {"params", {
            {"instrument_name", instrument},
            {"amount", amount},
            {"price", price},
            {"post_only", true}
        }}
    };

    std::string endpoint = "https://test.deribit.com/api/v2/private/" + orderType;
    std::string response = sendRequest(endpoint, payload, accessToken);

    if (response.empty()) {
        std::cerr << "Failed to receive a response or response is empty." << std::endl;
        return "Failed to receive a response or response is empty.";
    }

    auto responseJson = json::parse(response, nullptr, false);
    if (responseJson.is_discarded()) {
        std::cerr << "Failed to parse JSON response." << std::endl;
        return "Failed to parse JSON response.";
    }

    if (responseJson.contains("result")) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // Extract relevant order details
        const auto& order = responseJson["result"]["order"]; // Access the nested "order" object
        std::ostringstream output;
        output << "Order placed successfully:\n";
        output << "Order ID: " << order.value("order_id", "N/A") << "\n";
        output << "Instrument: " << order.value("instrument_name", "N/A") << "\n";
        output << "Price: " << (order.contains("price") ? std::to_string(order["price"].get<double>()) : std::string("N/A")) << "\n";
        output << "Amount: " << (order.contains("amount") ? std::to_string(order["amount"].get<double>()) : std::string("N/A")) << "\n";
        output << "Direction: " << order.value("direction", "N/A") << "\n";
        output << "Order State: " << order.value("order_state", "N/A") << "\n";
        output << "Time In Force: " << order.value("time_in_force", "N/A") << "\n";
        output << "Order Placement Latency: " << latency << " ms" << "\n";

        return output.str();

    } else {
        std::cerr << "Failed to place order." << std::endl;
        return "Failed to place order.";
    }
}


std::string cancelOrder(const std::string& orderId, const std::string& accessToken) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 2},
        {"method", "private/cancel"},
        {"params", {
            {"order_id", orderId}
        }}
    };

    std::string response = sendRequest("https://test.deribit.com/api/v2/private/cancel", payload, accessToken);
    auto responseJson = json::parse(response, nullptr, false);

    if (responseJson.contains("result")) {
        auto result = responseJson["result"];
        std::ostringstream output;
        output << "Order canceled successfully:\n";
        output << "Order ID: " << result["order_id"] << std::endl;
        output << "Instrument Name: " << result["instrument_name"] << std::endl;
        output << "Amount: " << result["amount"] << std::endl;
        output << "Price: " << result["price"] << std::endl;
        output << "Order State: " << result["order_state"] << std::endl;
        output << "Cancel Reason: " << result["cancel_reason"] << std::endl;

        // Include any other fields you consider relevant
        return output.str();

    }else {
        std::cerr << "Failed to cancel order." << std::endl;
        return "Failed to cancel order." ;
    }
}

std::string modifyOrder(const std::string& orderId, const std::string& newPrice, const std::string& accessToken,int amount) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 3},
        {"method", "private/edit"},
        {"params", {
            {"order_id", orderId},
            {"amount",amount},
            {"price", newPrice}
        }}
    };

    std::string response = sendRequest("https://test.deribit.com/api/v2/private/edit", payload, accessToken);
    auto responseJson = json::parse(response, nullptr, false);
    if (responseJson.contains("result")) {
        auto result = responseJson["result"];

        // Extract order details from result["order"]
        if (result.contains("order")) {
            std::ostringstream output;
            auto order = result["order"];
            output << "Order modified successfully.\n";
            // Extract relevant fields
            if (order.contains("order_id"))
                output << "Order ID: " << order["order_id"] << std::endl;
            if (order.contains("instrument_name"))
                output << "Instrument Name: " << order["instrument_name"] << std::endl;
            if (order.contains("amount"))
                output << "Amount: " << order["amount"] << std::endl;
            if (order.contains("price"))
                output << "Price: " << order["price"] << std::endl;
            if (order.contains("order_state"))
                output << "Order State: " << order["order_state"] << std::endl;
            if (order.contains("replaced"))
                output << "Replaced: " << (order["replaced"].get<bool>() ? "Yes" : "No") << std::endl;
            // Add any other fields you consider relevant
            return output.str();
        } else {
            std::cerr << "Error: 'order' not found in 'result'.\n";
        }
        return "Failed to modify order.";
    } else {
        std::cerr << "Failed to modify order." << std::endl;
        return "Failed to modify order." ;
    }
}

std::string getOrderBook(const std::string& instrument, const std::string& accessToken) { 
    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 4},
        {"method", "public/get_order_book"},
        {"params", {
            {"instrument_name", instrument}
        }}
    };

    std::string response = sendRequest("https://test.deribit.com/api/v2/public/get_order_book", payload, accessToken);
    auto responseJson = json::parse(response, nullptr, false);

    if (responseJson.contains("result")) {
        auto result = responseJson["result"];
        std::ostringstream output;

        output << "=== Order Book for " << instrument << " ===\n";

        output << "Best Bids:\n";
        for (const auto& bid : result["bids"]) {
            output << "Price: " << bid[0] << ", Quantity: " << bid[1] << '\n';
        }

        output << "\nBest Asks:\n";
        for (const auto& ask : result["asks"]) {
            output << "Price: " << ask[0] << ", Quantity: " << ask[1] << '\n';
        }

        output << "\nLast Price: " << result["last_price"] << "\n";

        return output.str();
    } else {
        return "Failed to retrieve order book.";
    }
}


std::string viewCurrentPosition(const std::string& accessToken, const std::string& instrument) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 5},
        {"method", "private/get_positions"},
        {"params", {
            {"instrument_name", instrument}
        }}
    };

    std::string response = sendRequest("https://test.deribit.com/api/v2/private/get_positions", payload, accessToken);
    auto responseJson = json::parse(response, nullptr, false);

    if (responseJson.contains("result")) {
        auto positions = responseJson["result"];
        std::ostringstream output;

        output << "=== Current Positions for " << instrument << " ===\n";

        for (const auto& position : positions) {
            output << "Instrument Name: " << position["instrument_name"] << '\n';
            output << "Size Currency: " << position["size_currency"] << '\n';
            output << "Average Price: " << position["average_price"] << '\n';
            output << "Mark Price: " << position["mark_price"] << '\n';
            output << "-------------------------------\n";
        }

        return output.str();
    } else {
        return "Failed to retrieve current positions.";
    }
}


std::string getOpenOrders(const std::string& accessToken) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 6},
        {"method", "private/get_open_orders"},
        {"params", {{"kind", "future"}, {"type", "limit"}}},
    };

    std::string response = sendRequest("https://test.deribit.com/api/v2/private/get_open_orders", payload, accessToken);
    auto responseJson = json::parse(response, nullptr, false);

    if (responseJson.contains("result")) {
        auto orders = responseJson["result"];
        if (orders.empty()) {
            return "No open orders found.\n";
        }

        std::ostringstream output;
        output << "=== Open Orders ===\n";

        for (const auto& order : orders) {
            output << "Order ID: " << order["order_id"] << '\n';
            output << "Instrument Name: " << order["instrument_name"] << '\n';
            output << "Price: " << order["price"] << '\n';
            output << "Amount: " << order["amount"] << '\n';
            output << "Direction: " << order["direction"] << '\n';
            output << "Order State: " << order["order_state"] << '\n';
            output << "Post Only: " << (order["post_only"].get<bool>() ? "Yes" : "No") << '\n';
            output << "Time In Force: " << order["time_in_force"] << '\n';
            output << "-------------------------------\n";
        }

        return output.str();
    } else {
        return "Failed to retrieve open orders.";
    }
}


void fetchOrderBookUpdates(WebSocketServer& server, const std::string& accessToken) {
    while (true) {
        // Sleep for a while to avoid API rate limits
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Lock the subscribers map
        std::map<std::string, std::vector<std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>>> subscribersCopy;
        {
            std::lock_guard<std::mutex> lock(server.subscribersMutex);
            subscribersCopy = server.subscribers; // Create a copy to avoid blocking the server
        }

        for (const auto& [symbol, clients] : subscribersCopy) {
            // Fetch the order book for each symbol
            json payload = {
                {"jsonrpc", "2.0"},
                {"id", 4},
                {"method", "public/get_order_book"},
                {"params", {{"instrument_name", symbol}}}
            };

            std::string response = sendRequest("https://test.deribit.com/api/v2/public/get_order_book", payload, accessToken);

            if (response.empty()) {
                std::cerr << "Failed to fetch order book for " << symbol << std::endl;
                continue;
            }

            auto responseJson = json::parse(response, nullptr, false);
            if (responseJson.is_discarded() || !responseJson.contains("result")) {
                std::cerr << "Invalid response for order book: " << response << std::endl;
                continue;
            }

        std::ostringstream orderBookOutput;

        // Extract necessary fields
        const auto& result = responseJson["result"];
        std::string instrumentName = result.value("instrument_name", "N/A");
        double bestBidPrice = result.value("best_bid_price", 0.0);
        double bestAskPrice = result.value("best_ask_price", 0.0);
        double lastPrice = result.value("last_price", 0.0);

        // Get a few top bids and asks
        std::vector<std::pair<double, double>> bids;
        std::vector<std::pair<double, double>> asks;

        if (result.contains("bids")) {
            for (const auto& bid : result["bids"]) {
                if (bid.is_array() && bid.size() == 2) {
                    bids.emplace_back(bid[0].get<double>(), bid[1].get<double>());
                }
                if (bids.size() >= 5) break; // Limit to the top 5 bids
            }
        }

        if (result.contains("asks")) {
            for (const auto& ask : result["asks"]) {
                if (ask.is_array() && ask.size() == 2) {
                    asks.emplace_back(ask[0].get<double>(), ask[1].get<double>());
                    // std::cout<<ask[0].get<double>()<<" "<<ask[1].get<double>()<<std::endl;
                }
                if (asks.size() >= 5) break; // Limit to the top 5 asks
            }
        }

        // Format the extracted data
        orderBookOutput << "Instrument: " << instrumentName << "\n";
        // std::cout<< "Instrument: " << instrumentName << "\n";

        orderBookOutput << "Best Bid: " << bestBidPrice << ", Best Ask: " << bestAskPrice << "\n";
        // std::cout<< "Best Bid: " << bestBidPrice << ", Best Ask: " << bestAskPrice << "\n";

        orderBookOutput << "Last Price: " << lastPrice << "\n";
        // std::cout<< "Last Price: " << lastPrice << "\n";


        orderBookOutput << "Top 5 Bids:\n";
        for (const auto& bid : bids) {
            orderBookOutput << "  Price: " << bid.first << ", Amount: " << bid.second << "\n";
        }

        orderBookOutput << "Top 5 Asks:\n";
        for (const auto& ask : asks) {
            orderBookOutput << "  Price: " << ask.first << ", Amount: " << ask.second << "\n";
            // std::cout<< "  Price: " << ask.first << ", Amount: " << ask.second << "\n";
        }

        // Convert to string and send the formatted data
        std::string formattedOrderBook = orderBookOutput.str();
        server.broadcastOrderBookUpdates(symbol, formattedOrderBook);



        }
    }
}