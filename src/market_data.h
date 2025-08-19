#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <deque>
#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace BitcoinTrader {

// Price data structure for a single candlestick
struct Kline {
    long long timestamp;    // Unix timestamp in milliseconds
    double open;           // Opening price
    double high;           // Highest price
    double low;            // Lowest price
    double close;          // Closing price
    double volume;         // Trading volume
    
    // Constructor
    Kline(long long ts = 0, double o = 0.0, double h = 0.0, 
          double l = 0.0, double c = 0.0, double v = 0.0)
        : timestamp(ts), open(o), high(h), low(l), close(c), volume(v) {}
    
    // Utility functions
    double typical_price() const { return (high + low + close) / 3.0; }
    double hl2() const { return (high + low) / 2.0; }
    double hlc3() const { return (high + low + close) / 3.0; }
    double ohlc4() const { return (open + high + low + close) / 4.0; }
    double price_range() const { return high - low; }
    
    // Validation
    bool is_valid() const {
        return timestamp > 0 && open > 0 && high > 0 && low > 0 && 
               close > 0 && volume >= 0 && high >= low && 
               high >= open && high >= close && low <= open && low <= close;
    }
};

// Time series data container with efficient operations
class TimeSeries {
private:
    std::deque<Kline> data_;
    size_t max_size_;
    bool auto_trim_;

public:
    // Constructors
    TimeSeries(size_t max_size = 1000, bool auto_trim = true);
    TimeSeries(const std::vector<Kline>& initial_data, size_t max_size = 1000, bool auto_trim = true);
    
    // Data management
    void add_kline(const Kline& kline);
    void add_klines(const std::vector<Kline>& klines);
    void clear();
    void reserve(size_t size);
    
    // Access methods
    const Kline& operator[](size_t index) const;
    const Kline& at(size_t index) const;
    const Kline& latest() const;
    const Kline& oldest() const;
    
    // Iterator support
    std::deque<Kline>::const_iterator begin() const { return data_.begin(); }
    std::deque<Kline>::const_iterator end() const { return data_.end(); }
    std::deque<Kline>::const_reverse_iterator rbegin() const { return data_.rbegin(); }
    std::deque<Kline>::const_reverse_iterator rend() const { return data_.rend(); }
    
    // Size and capacity
    size_t size() const { return data_.size(); }
    size_t max_size() const { return max_size_; }
    bool empty() const { return data_.empty(); }
    bool is_full() const { return data_.size() >= max_size_; }
    
    // Data extraction methods
    std::vector<double> get_closes(size_t count = 0) const;
    std::vector<double> get_opens(size_t count = 0) const;
    std::vector<double> get_highs(size_t count = 0) const;
    std::vector<double> get_lows(size_t count = 0) const;
    std::vector<double> get_volumes(size_t count = 0) const;
    std::vector<double> get_typical_prices(size_t count = 0) const;
    
    // Time-based queries
    std::vector<Kline> get_range(long long start_time, long long end_time) const;
    std::vector<Kline> get_last_n_klines(size_t n) const;
    
    // Statistical methods
    double get_price_change(size_t periods = 1) const;
    double get_price_change_percent(size_t periods = 1) const;
    double get_average_volume(size_t periods = 0) const;
    double get_price_volatility(size_t periods = 0) const;
    
    // Validation
    bool validate_data() const;
    size_t count_invalid_klines() const;
    
    // Configuration
    void set_max_size(size_t new_max_size);
    void set_auto_trim(bool auto_trim);
};

// Market data aggregator and manager
class MarketDataManager {
private:
    std::map<std::string, std::unique_ptr<TimeSeries>> symbol_data_;
    std::map<std::string, std::string> symbol_intervals_;
    
public:
    // Constructor
    MarketDataManager() = default;
    
    // Symbol management
    void register_symbol(const std::string& symbol, const std::string& interval, 
                        size_t max_size = 1000);
    void unregister_symbol(const std::string& symbol);
    bool has_symbol(const std::string& symbol) const;
    std::vector<std::string> get_registered_symbols() const;
    
    // Data operations
    void add_kline(const std::string& symbol, const Kline& kline);
    void add_klines(const std::string& symbol, const std::vector<Kline>& klines);
    void update_latest_price(const std::string& symbol, double price);
    
    // Data access
    const TimeSeries* get_time_series(const std::string& symbol) const;
    TimeSeries* get_time_series_mutable(const std::string& symbol);
    
    const Kline& get_latest_kline(const std::string& symbol) const;
    double get_latest_price(const std::string& symbol) const;
    std::vector<double> get_recent_closes(const std::string& symbol, size_t count) const;
    
    // Market statistics
    std::map<std::string, double> get_all_latest_prices() const;
    std::map<std::string, double> get_price_changes_24h() const;
    std::map<std::string, size_t> get_data_counts() const;
    
    // Utility methods
    void clear_all_data();
    void clear_symbol_data(const std::string& symbol);
    std::string get_market_summary() const;
};

// Price level for order book or support/resistance
struct PriceLevel {
    double price;
    double volume;
    long long timestamp;
    
    PriceLevel(double p = 0.0, double v = 0.0, long long ts = 0)
        : price(p), volume(v), timestamp(ts) {}
};

// Order book snapshot (for future advanced features)
struct OrderBook {
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    long long timestamp;
    
    OrderBook() : timestamp(0) {}
    
    double get_mid_price() const {
        if (bids.empty() || asks.empty()) return 0.0;
        return (bids[0].price + asks[0].price) / 2.0;
    }
    
    double get_spread() const {
        if (bids.empty() || asks.empty()) return 0.0;
        return asks[0].price - bids[0].price;
    }
};

// Trading session information
struct TradingSession {
    std::string symbol;
    long long start_time;
    long long end_time;
    double start_price;
    double end_price;
    double high_price;
    double low_price;
    double total_volume;
    size_t total_trades;
    
    TradingSession(const std::string& sym = "")
        : symbol(sym), start_time(0), end_time(0), start_price(0.0),
          end_price(0.0), high_price(0.0), low_price(0.0),
          total_volume(0.0), total_trades(0) {}
    
    double get_return() const {
        return start_price > 0 ? ((end_price - start_price) / start_price) * 100.0 : 0.0;
    }
    
    double get_volatility() const {
        return start_price > 0 ? ((high_price - low_price) / start_price) * 100.0 : 0.0;
    }
};

// Utility functions
namespace MarketDataUtils {
    // Time conversion utilities
    long long current_timestamp_ms();
    std::string timestamp_to_string(long long timestamp);
    long long string_to_timestamp(const std::string& time_str);
    
    // Data validation
    bool is_valid_price(double price);
    bool is_valid_volume(double volume);
    bool is_valid_timestamp(long long timestamp);
    
    // Data analysis helpers
    double calculate_sma(const std::vector<double>& prices, size_t period);
    double calculate_ema(const std::vector<double>& prices, size_t period);
    double calculate_stddev(const std::vector<double>& values);
    double calculate_variance(const std::vector<double>& values);
    
    // Price formatting
    std::string format_price(double price, int decimal_places = 2);
    std::string format_volume(double volume);
    std::string format_percentage(double percentage);
}

} // namespace BitcoinTrader