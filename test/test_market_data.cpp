#include "../src/market_data.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <ctime>

using namespace BitcoinTrader;

// Test helper functions
void assert_equal(double expected, double actual, double tolerance = 1e-6) {
    if (std::abs(expected - actual) > tolerance) {
        std::cerr << "Assertion failed: expected " << expected 
                  << ", got " << actual << std::endl;
        assert(false);
    }
}

void assert_true(bool condition, const std::string& message = "") {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << std::endl;
        assert(false);
    }
}

// Test Kline struct
void test_kline_basic() {
    std::cout << "Testing Kline basic functionality..." << std::endl;
    
    // Test constructor
    Kline kline(1234567890000LL, 50000.0, 51000.0, 49500.0, 50500.0, 1.5);
    
    assert_equal(50000.0, kline.open);
    assert_equal(51000.0, kline.high);
    assert_equal(49500.0, kline.low);
    assert_equal(50500.0, kline.close);
    assert_equal(1.5, kline.volume);
    
    // Test utility functions
    assert_equal(50333.333333, kline.typical_price(), 1e-5);
    assert_equal(50250.0, kline.hl2());
    assert_equal(50333.333333, kline.hlc3(), 1e-5);
    assert_equal(50250.0, kline.ohlc4());
    assert_equal(1500.0, kline.price_range());
    
    // Test validation
    assert_true(kline.is_valid(), "Valid kline should pass validation");
    
    // Test invalid kline
    Kline invalid_kline(0, -100.0, 50000.0, 49000.0, 50500.0, 1.0);
    assert_true(!invalid_kline.is_valid(), "Invalid kline should fail validation");
    
    std::cout << "✓ Kline tests passed" << std::endl;
}

// Test TimeSeries functionality
void test_time_series() {
    std::cout << "Testing TimeSeries functionality..." << std::endl;
    
    TimeSeries ts(5, true); // max size 5, auto-trim enabled
    
    // Test empty state
    assert_true(ts.empty(), "New TimeSeries should be empty");
    assert_equal(0, ts.size());
    
    // Add some test data
    std::vector<Kline> test_data = {
        Kline(1000, 100.0, 105.0, 95.0, 102.0, 1.0),
        Kline(2000, 102.0, 108.0, 100.0, 105.0, 1.2),
        Kline(3000, 105.0, 110.0, 103.0, 107.0, 0.8),
        Kline(4000, 107.0, 112.0, 105.0, 110.0, 1.5),
        Kline(5000, 110.0, 115.0, 108.0, 112.0, 0.9)
    };
    
    ts.add_klines(test_data);
    
    assert_equal(5, ts.size());
    assert_true(ts.is_full(), "TimeSeries should be full");
    
    // Test data access
    assert_equal(102.0, ts[0].close);
    assert_equal(112.0, ts.latest().close);
    assert_equal(102.0, ts.oldest().close);
    
    // Test data extraction
    auto closes = ts.get_closes();
    assert_equal(5, closes.size());
    assert_equal(102.0, closes[0]);
    assert_equal(112.0, closes[4]);
    
    auto last_3_closes = ts.get_closes(3);
    assert_equal(3, last_3_closes.size());
    assert_equal(107.0, last_3_closes[0]);
    assert_equal(112.0, last_3_closes[2]);
    
    // Test price change
    double change = ts.get_price_change(1);
    assert_equal(2.0, change); // 112 - 110
    
    double change_percent = ts.get_price_change_percent(4);
    double expected_percent = ((112.0 - 102.0) / 102.0) * 100.0;
    assert_equal(expected_percent, change_percent, 1e-5);
    
    // Test auto-trimming by adding more data
    Kline extra_kline(6000, 112.0, 118.0, 110.0, 115.0, 1.1);
    ts.add_kline(extra_kline);
    
    assert_equal(5, ts.size()); // Should still be 5 due to auto-trim
    assert_equal(105.0, ts.oldest().close); // First kline should be removed
    assert_equal(115.0, ts.latest().close);
    
    std::cout << "✓ TimeSeries tests passed" << std::endl;
}

// Test MarketDataManager
void test_market_data_manager() {
    std::cout << "Testing MarketDataManager..." << std::endl;
    
    MarketDataManager manager;
    
    // Test symbol registration
    manager.register_symbol("BTCUSDT", "1m", 100);
    manager.register_symbol("ETHUSDT", "1m", 100);
    
    assert_true(manager.has_symbol("BTCUSDT"), "BTCUSDT should be registered");
    assert_true(manager.has_symbol("ETHUSDT"), "ETHUSDT should be registered");
    assert_true(!manager.has_symbol("ADAUSDT"), "ADAUSDT should not be registered");
    
    auto symbols = manager.get_registered_symbols();
    assert_equal(2, symbols.size());
    
    // Test adding data
    std::vector<Kline> btc_data = {
        Kline(1000, 50000.0, 51000.0, 49500.0, 50500.0, 1.0),
        Kline(2000, 50500.0, 52000.0, 50000.0, 51500.0, 1.2),
        Kline(3000, 51500.0, 53000.0, 51000.0, 52000.0, 0.8)
    };
    
    manager.add_klines("BTCUSDT", btc_data);
    
    // Test data retrieval
    assert_equal(52000.0, manager.get_latest_price("BTCUSDT"));
    
    auto recent_closes = manager.get_recent_closes("BTCUSDT", 2);
    assert_equal(2, recent_closes.size());
    assert_equal(51500.0, recent_closes[0]);
    assert_equal(52000.0, recent_closes[1]);
    
    // Test all latest prices
    auto all_prices = manager.get_all_latest_prices();
    assert_equal(1, all_prices.size()); // Only BTCUSDT has data
    assert_equal(52000.0, all_prices["BTCUSDT"]);
    
    // Test data counts
    auto counts = manager.get_data_counts();
    assert_equal(3, counts["BTCUSDT"]);
    assert_equal(0, counts["ETHUSDT"]);
    
    // Test market summary
    std::string summary = manager.get_market_summary();
    assert_true(!summary.empty(), "Market summary should not be empty");
    
    std::cout << "✓ MarketDataManager tests passed" << std::endl;
}

// Test utility functions
void test_market_data_utils() {
    std::cout << "Testing MarketDataUtils..." << std::endl;
    
    // Test price validation
    assert_true(MarketDataUtils::is_valid_price(100.0));
    assert_true(!MarketDataUtils::is_valid_price(-100.0));
    assert_true(!MarketDataUtils::is_valid_price(0.0));
    
    // Test volume validation
    assert_true(MarketDataUtils::is_valid_volume(0.0));
    assert_true(MarketDataUtils::is_valid_volume(100.0));
    assert_true(!MarketDataUtils::is_valid_volume(-100.0));
    
    // Test timestamp validation
    assert_true(MarketDataUtils::is_valid_timestamp(1234567890000LL));
    assert_true(!MarketDataUtils::is_valid_timestamp(0));
    assert_true(!MarketDataUtils::is_valid_timestamp(-1000));
    
    // Test SMA calculation
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0};
    double sma = MarketDataUtils::calculate_sma(prices, 3);
    assert_equal(106.0, sma); // (104 + 106 + 108) / 3
    
    // Test standard deviation
    std::vector<double> values = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    double stddev = MarketDataUtils::calculate_stddev(values);
    assert_equal(2.138, stddev, 0.2); // Approximately 2.14
    
    // Test formatting
    std::string price_str = MarketDataUtils::format_price(1234.567, 2);
    assert_true(price_str.find("$1234.57") != std::string::npos);
    
    std::string volume_str = MarketDataUtils::format_volume(1500000.0);
    assert_true(volume_str.find("1.50M") != std::string::npos);
    
    std::string percent_str = MarketDataUtils::format_percentage(5.25);
    assert_true(percent_str.find("+5.25%") != std::string::npos);
    
    std::cout << "✓ MarketDataUtils tests passed" << std::endl;
}

// Test performance with larger datasets
void test_performance() {
    std::cout << "Testing performance with larger dataset..." << std::endl;
    
    TimeSeries ts(10000, true);
    
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    // Generate 10,000 klines
    for (int i = 0; i < 10000; ++i) {
        double base_price = 50000.0 + (i * 0.1);
        Kline kline(
            i * 1000,                    // timestamp
            base_price,                  // open
            base_price + 10.0,          // high
            base_price - 10.0,          // low
            base_price + 5.0,           // close
            1.0 + (i % 10) * 0.1        // volume
        );
        ts.add_kline(kline);
    }
    
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Added 10,000 klines in " << duration.count() << "ms" << std::endl;
    
    // Test data extraction performance
    start = std::chrono::high_resolution_clock::now();
    auto closes = ts.get_closes(1000);
    end = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds duration_micro = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Extracted 1,000 closes in " << duration_micro.count() << "μs" << std::endl;
    
    assert_equal(1000, closes.size());
    assert_equal(10000, ts.size());
    
    std::cout << "✓ Performance tests passed" << std::endl;
}

// Integration test with realistic Bitcoin data simulation
void test_bitcoin_simulation() {
    std::cout << "Testing Bitcoin price simulation..." << std::endl;
    
    MarketDataManager manager;
    manager.register_symbol("BTCUSDT", "1m", 1440); // 24 hours of 1m data
    
    // Simulate realistic Bitcoin price movements
    double base_price = 50000.0;
    long long timestamp = 1640995200000LL; // Jan 1, 2022
    
    std::vector<Kline> simulation_data;
    
    for (int i = 0; i < 100; ++i) {
        // Simulate smaller, more realistic volatility
        double price_change_percent = (std::rand() % 100 - 50) * 0.01; // -0.5% to +0.5%
        double new_close = base_price * (1.0 + price_change_percent / 100.0);
        
        // Ensure new_close is positive
        if (new_close <= 0) {
            new_close = base_price * 0.999; // Small decrease if random goes too negative
        }
        
        // Create realistic OHLC with proper relationships
        double volatility_factor = 0.002; // 0.2% max intraday movement
        double high_offset = (std::rand() % 100) * 0.00001 * new_close; // Up to 0.1% higher
        double low_offset = (std::rand() % 100) * 0.00001 * new_close;  // Up to 0.1% lower
        
        double high = std::max(base_price, new_close) + high_offset;
        double low = std::min(base_price, new_close) - low_offset;
        
        // Ensure proper OHLC relationships
        if (low <= 0) low = std::min(base_price, new_close) * 0.999;
        if (high <= low) high = low * 1.001;
        if (base_price > high) high = base_price * 1.001;
        if (base_price < low) low = base_price * 0.999;
        if (new_close > high) high = new_close * 1.001;
        if (new_close < low) low = new_close * 0.999;
        
        double volume = 1.0 + (std::rand() % 100) * 0.01; // 1.0 to 2.0
        
        Kline kline(timestamp + i * 60000, base_price, high, low, new_close, volume);
        
        // Validate before adding
        if (kline.is_valid()) {
            simulation_data.push_back(kline);
        } else {
            // Create a simple valid kline if validation fails
            Kline fallback_kline(timestamp + i * 60000, base_price, base_price * 1.001, base_price * 0.999, base_price, 1.0);
            simulation_data.push_back(fallback_kline);
        }
        
        base_price = new_close;
    }
    
    manager.add_klines("BTCUSDT", simulation_data);
    
    // Verify simulation data
    auto ts = manager.get_time_series("BTCUSDT");
    assert_true(ts != nullptr);
    assert_equal(100, ts->size());
    
    // Check that most data is valid (allow for some edge cases)
    size_t invalid_count = ts->count_invalid_klines();
    assert_true(invalid_count == 0, "All simulation data should be valid");
    
    // Calculate some statistics
    double latest_price = manager.get_latest_price("BTCUSDT");
    double price_change = ts->get_price_change_percent(99); // Full period change
    double volatility = ts->get_price_volatility();
    
    std::cout << "Simulation results:" << std::endl;
    std::cout << "  Latest price: " << MarketDataUtils::format_price(latest_price) << std::endl;
    std::cout << "  Total change: " << MarketDataUtils::format_percentage(price_change) << std::endl;
    std::cout << "  Volatility: " << volatility << std::endl;
    
    assert_true(latest_price > 0, "Latest price should be positive");
    assert_true(std::abs(price_change) < 10, "Price change should be reasonable"); // Increased tolerance
    
    std::cout << "✓ Bitcoin simulation tests passed" << std::endl;
}

int main() {
    std::cout << "Running Market Data Module Tests" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    try {
        test_kline_basic();
        test_time_series();
        test_market_data_manager();
        test_market_data_utils();
        test_performance();
        test_bitcoin_simulation();
        
        std::cout << std::endl;
        std::cout << "🎉 All tests passed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}