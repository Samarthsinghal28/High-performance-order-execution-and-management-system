#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <iostream>
#include <nlohmann/json.hpp>
#include "../websocket/server.h"

using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
std::string sendRequest(const std::string& url, const json& payload, const std::string& accessToken = "");
std::string getAccessToken(const std::string& clientId, const std::string& clientSecret);
std::string placeOrder(const std::string& orderType, const std::string& price, const std::string& accessToken, const std::string& amount, const std::string& instrument);
std::string cancelOrder(const std::string& orderId, const std::string& accessToken);
std::string modifyOrder(const std::string& orderId, const std::string& newPrice, const std::string& accessToken,int amount);
std::string getOrderBook(const std::string& instrument, const std::string& accessToken);
std::string viewCurrentPosition(const std::string& accessToken, const std::string& instrument);
std::string getOpenOrders(const std::string& accessToken);
void fetchOrderBookUpdates(WebSocketServer& server, const std::string& accessToken);


#endif // FUNCTIONS_H

