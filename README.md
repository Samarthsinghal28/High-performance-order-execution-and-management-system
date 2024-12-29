# High Performance Order Execution and Management System


## Overview

This project is a C++ implementation of a trading system that interacts with the [Deribit](https://www.deribit.com/) cryptocurrency exchange. It allows users to perform order management functions, such as placing, canceling, and modifying orders, and provides real-time market data streaming via a custom WebSocket server.

## Table of Contents

- [Initial Setup](#initial-setup)
- [Features](#features)
- [Market Coverage](#market-coverage)
- [Technical Requirements](#technical-requirements)
- [Project Structure](#project-structure)
- [Installation](#installation)
- [Usage](#usage)
  - [Running the Server](#running-the-server)
  - [Running the Client](#running-the-client)
- [Configuration](#configuration)
- [Example Commands](#example-commands)
- [Contributing](#contributing)
- [License](#license)

## Initial Setup

1. **Create a new Deribit Testnet Account**
   - Visit the [Deribit Testnet](https://test.deribit.com/) website and sign up for a test account.

2. **Generate API Keys for Authentication**
   - Navigate to **Account > API** in your Deribit test account.
   - Generate new API keys with the necessary permissions (e.g., `trade`, `read`).

## Key Features

### **Order Execution and Management**

1. **Place Order**
2. **Cancel Order**
3. **Modify Order**
4. **Get Order Book**
5. **View Current Positions**
6. **Real-time Market Data Streaming via WebSocket**
   - Custom WebSocket server functionality.
   - Allows clients to subscribe to trading symbols.
   - Streams continuous order book updates for subscribed symbols.
   - Place, cancel, modify orders using the websocket messages.


## Technology Stack
- **C++**: Core implementation language.
- **Boost.Asio and Boost.Beast**: For WebSocket server functionality.
- **cURL**: For making HTTP requests to the Deribit API.
- **nlohmann/json**: For JSON parsing and manipulation.

## Project Structure
```
project_root/
├── main.cpp             # Entry point for the application.
├── client.cpp           # Program to test the data from the web socket.
├── websocket/           # Contains WebSocket server logic.
│   ├── server.h         # Header file for WebSocket server class.
│   └── server.cpp       # Implementation of WebSocket server functions.
├── functions
│   ├── functions.h      # Header file for order execution functions.
│   └── functions.cpp    # Implementation of order execution functions.
```

## Getting Started

### Prerequisites
- **C++ Compiler**: GCC 7.0+ or Clang with C++11 support.
- **Libraries:**
  - Boost (Asio and Beast)
  - cURL
  - nlohmann/json
- **API Credentials:** Generate `client_id` and `client_secret` from the [Deribit Testnet](https://test.deribit.com/).

### Building the Project
1. Clone the repository:
   ```bash
   https://github.com/Samarthsinghal28/High-performance-order-execution-and-management-system
   cd High-performance-order-execution-and-management-system
   ```
2. Compile the code:
   ```bash
   g++ -std=c++17 main.cpp -o trading_system  -lboost_system -lcurl
   ```
   ```bash
   g++ -std=c++17 client.cpp -o client -lboost_system -lpthread
   ```

### Running the Server
1. Execute the compiled binary:
   ```bash
   ./trading_system
   ```
2. The WebSocket server will run on the specified port, and trading operations will execute based on the implemented logic in `main.cpp`.

### Running the Client
1. Execute the compiled binary:
   ```bash
   ./client
   ```
   - The client connects to the WebSocket server at 127.0.0.1:8080.

2. Subscribe to Market Data:
    - The client automatically subscribes to a default symbol (e.g., BTC-PERPETUAL).
    - To subscribe to a different symbol, modify the client.cpp file, or extend it to accept user input.

3. The client displays incoming order book updates along with propagation delay:
    ```
    Instrument: BTC-PERPETUAL
    Best Bid: 50000, Best Ask: 50010
    Last Price: 50005
    Top 5 Bids:
    Price: 50000, Amount: 1.0
    ...
    Top 5 Asks:
    Price: 50010, Amount: 1.5
    ...
    Propagation Delay: 500 ns
    ```

## Example Commands

### Placing an Order
   - Place a Buy Order :

```
    Enter your choice: 1
    Enter order details (type price amount instrument): buy 50000 10 BTC-PERPETUAL
```
   - Output:

```
    Order placed successfully:
    Order ID: 30589360463
    Instrument: BTC-PERPETUAL
    Price: 50000.000000
    Amount: 10.000000
    Direction: buy
    Order State: open
    Time In Force: good_til_cancelled
    Order Response: Order Placement Latency: 772 ms
```

### Canceling an Order

   - Cancel an Order by ID :

```
    Enter your choice: 2
    Enter Order ID to cancel: 30589360463
```
   - Output:

```
    Order canceled successfully:
    Order ID: 30589360463
    Instrument Name: BTC-PERPETUAL
    Amount: 10.0
    Price: 50000.0
    Order State: cancelled
    Cancel Reason: user_request
```

### Modifying an Order

   - Modify an Existing Order :

```
    Enter your choice: 3
    Enter Order ID and new price: 30589360463 49900
```
   - Output:

```
    Order modified successfully.
    Order ID: 30589360463
    Instrument Name: BTC-PERPETUAL
    Amount: 10
    Price: 49900
    Order State: open
    Replaced: Yes
```

### Viewing Open Orders

   - List All Open Orders :

```
    Enter your choice: 4
```
   - Output:

```
    === Open Orders ===
    Order ID: 30589360463
    Instrument Name: BTC-PERPETUAL
    Price: 49900
    Amount: 10
    Direction: buy
    Order State: open
    Post Only: Yes
    Time In Force: good_til_cancelled
    -------------------------------
```


### Viewing Current Positions

   - Check Positions for an Instrument :

```
    Enter your choice: 5
    Enter instrument name: BTC-PERPETUAL
```
   - Output:

```
    === Current Positions for BTC-PERPETUAL ===
    Instrument Name: BTC-PERPETUAL
    Size Currency: 0.0
    Average Price: 0.0
    Mark Price: 93558.37
    -------------------------------
```

### Exiting the Server Application

   - Quit the Server :

```
    Enter your choice: 6
```
    
## Client Output

```


```
    Connected to WebSocket server
    Subscription request sent for: BTC-PERPETUAL
    Enter 'quit' to exit
    Received (raw): Subscribed to: BTC-PERPETUAL
    Message: Instrument: BTC-PERPETUAL
    Best Bid: 93719, Best Ask: 93746
    Last Price: 93747.5
    Top 5 Bids:
    Price: 93719, Amount: 10
    Price: 93699, Amount: 10
    Price: 93679, Amount: 10
    Price: 93659, Amount: 10
    Price: 93639, Amount: 10
    Top 5 Asks:
    Price: 93746, Amount: 7.72639e+06
    Price: 93759, Amount: 10
    Price: 93928.5, Amount: 7.77777e+06
    Price: 94522.5, Amount: 10
    Price: 94537.5, Amount: 2000

    Propagation Delay: 615766 ns
    Message: Instrument: BTC-PERPETUAL
    Best Bid: 93719, Best Ask: 93746
    Last Price: 93747.5
    Top 5 Bids:
    Price: 93719, Amount: 10
    Price: 93699, Amount: 10
    Price: 93679, Amount: 10
    Price: 93659, Amount: 10
    Price: 93639, Amount: 10
    Top 5 Asks:
    Price: 93746, Amount: 7.72639e+06
    Price: 93759, Amount: 10
    Price: 93928.5, Amount: 7.77777e+06
    Price: 94522.5, Amount: 10
    Price: 94537.5, Amount: 2000

    Propagation Delay: 275859 ns

---
This project showcases advanced C++ programming skills, expertise in WebSocket development, and proficiency in API integration. It is designed with modularity and scalability, making it a robust solution for real-time trading systems.

