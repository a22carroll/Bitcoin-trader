#include <iostream>
#include "binance.h"


int main() {
    std::cout << "Starting Binance API Test..." << std::endl;
    
    // Create client (will load config.json automatically)
    binance::BinanceClient client(true); // true = use testnet
    
    std::cout << "Created Binance client" << std::endl;
    
    // Test 1: Connection test
    std::cout << "\n--- Test 1: Connection Test ---" << std::endl;
    if (client.test_connection()) {
        std::cout << "✓ Successfully connected to Binance API!" << std::endl;
    } else {
        std::cout << "✗ Failed to connect to Binance API" << std::endl;
        return 1;
    }
    
    // Test 2: Get Bitcoin price
    std::cout << "\n--- Test 2: Get Bitcoin Price ---" << std::endl;
    double btc_price = client.get_price("BTCUSDT");
    if (btc_price > 0) {
        std::cout << "✓ BTC Price: $" << btc_price << std::endl;
    } else {
        std::cout << "✗ Failed to get BTC price" << std::endl;
    }
    
    // Test 3: Get historical data
    std::cout << "\n--- Test 3: Get Historical Data ---" << std::endl;
    auto klines = client.get_klines("BTCUSDT", "1h", 5);
    if (!klines.empty()) {
        std::cout << "✓ Got " << klines.size() << " hourly candles:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(3), klines.size()); i++) {
            std::cout << "  Candle " << i+1 << ": Open=$" << klines[i].open 
                      << " High=$" << klines[i].high 
                      << " Low=$" << klines[i].low 
                      << " Close=$" << klines[i].close << std::endl;
        }
    } else {
        std::cout << "✗ Failed to get historical data" << std::endl;
    }
    
    // Test 4: Account info (only if you have API keys set)
    std::cout << "\n--- Test 4: Account Info (requires API keys) ---" << std::endl;
    auto balances = client.get_balances();
    if (!balances.empty()) {
        std::cout << "✓ Got account balances:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(5), balances.size()); i++) {
            std::cout << "  " << balances[i].asset << ": " 
                      << balances[i].free << " (free) + " 
                      << balances[i].locked << " (locked)" << std::endl;
        }
    } else {
        std::cout << "! No balances found (normal if API keys not set)" << std::endl;
    }
    
    std::cout << "\nTest completed!" << std::endl;
    return 0;
}