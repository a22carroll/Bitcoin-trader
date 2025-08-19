#ifndef BINANCE_H
#define BINANCE_H

#include <string>
#include <vector>

// The binance namespace keeps our code organized and prevents naming conflicts
namespace binance {

// Data structure to hold price information for any trading pair
struct Price {
    std::string symbol;  // Trading pair like "BTCUSDT" 
    double price;        // Current price in quote currency (USD for BTCUSDT)
};

// Data structure for candlestick/OHLC data - essential for technical analysis
struct Kline {
    long timestamp;      // Unix timestamp when this candle started
    double open;         // Opening price at start of time period
    double high;         // Highest price during time period
    double low;          // Lowest price during time period  
    double close;        // Closing price at end of time period
    double volume;       // Total volume traded during time period
};

// Data structure to hold account balance information
struct Balance {
    std::string asset;   // Currency/asset name like "BTC" or "USDT"
    double free;         // Amount available for trading
    double locked;       // Amount locked in open orders
};

// Main class that handles all communication with Binance API
class BinanceClient {
private:
    // Private member variables - only this class can access them
    std::string api_key_;      // Your API key from Binance
    std::string secret_key_;   // Your secret key for signing requests
    std::string base_url_;     // Base URL for API calls (testnet vs live)
    bool testnet_;            // Flag to use testnet (fake money) vs live trading

    

public:
    // Constructor - called when you create a BinanceClient object
    // testnet=true means use fake money by default (safer for development)
    BinanceClient(bool testnet = true);
    
    // Method to set your API credentials after creating the object
    void set_credentials(const std::string& api_key, const std::string& secret_key);
    
    // PUBLIC ENDPOINTS - These don't require API keys, anyone can call them
    
    // Get the current price of any trading pair (like "BTCUSDT")
    // Returns just the price as a double for simplicity
    double get_price(const std::string& symbol) const;
    
    // Get historical price data (candlesticks) for technical analysis
    // symbol: trading pair like "BTCUSDT"
    // interval: time period like "1m", "5m", "1h", "1d" 
    // limit: how many candles to get (max 1000)
    std::vector<Kline> get_klines(const std::string& symbol, 
                                  const std::string& interval, 
                                  int limit = 100) const;
    
    // PRIVATE ENDPOINTS - These require API keys and authentication
    
    // Get all your account balances (how much BTC, USDT, etc. you have)
    std::vector<Balance> get_balances() const;
    
    // Get balance for a specific asset (like "BTC" or "USDT")
    double get_balance(const std::string& asset) const;
    
    // UTILITY METHODS
    
    // Test if you can connect to Binance API (useful for debugging)
    bool test_connection() const;
    
private:
    // Private helper methods - only used internally by this class
    

    std::string create_signature(const std::string& data) const;
    long get_timestamp() const;
    
    // And the config loader:
    void load_config_from_file(const std::string& config_file);
    // Make a basic HTTP request to public endpoints
    std::string make_request(const std::string& endpoint) const;
    
    // Make an authenticated HTTP request to private endpoints
    // This adds your API key and creates a signature
    std::string make_signed_request(const std::string& endpoint) const;
};

} // namespace binance

#endif // BINANCE_H