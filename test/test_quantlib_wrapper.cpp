#include "quantlib_wrapper.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <iomanip>

using namespace BitcoinTrader;

// Test helper functions
bool is_close(double a, double b, double tolerance = 1e-6) {
    return std::abs(a - b) < tolerance;
}

void print_vector(const std::vector<double>& vec, const std::string& name, int precision = 4) {
    std::cout << name << ": [";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (std::isnan(vec[i])) {
            std::cout << "NaN";
        } else {
            std::cout << std::fixed << std::setprecision(precision) << vec[i];
        }
        if (i < vec.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
}

// Test data - simulating BTCUSDT price action
std::vector<double> create_test_closes() {
    return {
        50000.0, 50100.0, 49950.0, 50200.0, 50150.0,
        50300.0, 50250.0, 50400.0, 50350.0, 50500.0,
        50450.0, 50600.0, 50550.0, 50700.0, 50650.0,
        50800.0, 50750.0, 50900.0, 50850.0, 51000.0
    };
}

std::vector<double> create_test_highs() {
    return {
        50050.0, 50150.0, 50000.0, 50250.0, 50200.0,
        50350.0, 50300.0, 50450.0, 50400.0, 50550.0,
        50500.0, 50650.0, 50600.0, 50750.0, 50700.0,
        50850.0, 50800.0, 50950.0, 50900.0, 51050.0
    };
}

std::vector<double> create_test_lows() {
    return {
        49950.0, 50050.0, 49900.0, 50150.0, 50100.0,
        50250.0, 50200.0, 50350.0, 50300.0, 50450.0,
        50400.0, 50550.0, 50500.0, 50650.0, 50600.0,
        50750.0, 50700.0, 50850.0, 50800.0, 50950.0
    };
}

std::vector<double> create_test_volumes() {
    return {
        1000.0, 1200.0, 950.0, 1100.0, 1050.0,
        1300.0, 1150.0, 1400.0, 1250.0, 1500.0,
        1350.0, 1600.0, 1450.0, 1700.0, 1550.0,
        1800.0, 1650.0, 1900.0, 1750.0, 2000.0
    };
}

void test_moving_averages() {
    std::cout << "\n=== Testing Moving Averages ===\n";
    
    auto closes = create_test_closes();
    
    // Test SMA
    auto sma_5 = QuantLibWrapper::sma(closes, 5);
    std::cout << "SMA(5) test:\n";
    print_vector(sma_5, "SMA(5)", 2);
    
    // Verify SMA calculation manually for position 4 (5th element, index 4)
    double expected_sma = (closes[0] + closes[1] + closes[2] + closes[3] + closes[4]) / 5.0;
    assert(is_close(sma_5[4], expected_sma));
    std::cout << "✓ SMA calculation verified\n";
    
    // Test EMA
    auto ema_5 = QuantLibWrapper::ema(closes, 5);
    std::cout << "EMA(5) test:\n";
    print_vector(ema_5, "EMA(5)", 2);
    assert(!std::isnan(ema_5[4])); // Should have value at position 4
    std::cout << "✓ EMA calculation completed\n";
    
    // Test WMA
    auto wma_5 = QuantLibWrapper::wma(closes, 5);
    std::cout << "WMA(5) test:\n";
    print_vector(wma_5, "WMA(5)", 2);
    assert(!std::isnan(wma_5[4])); // Should have value at position 4
    std::cout << "✓ WMA calculation completed\n";
}

void test_rsi() {
    std::cout << "\n=== Testing RSI ===\n";
    
    auto closes = create_test_closes();
    auto rsi_14 = QuantLibWrapper::rsi(closes, 14);
    
    print_vector(rsi_14, "RSI(14)", 2);
    
    // RSI should be between 0 and 100
    for (size_t i = 14; i < rsi_14.size(); ++i) {
        if (!std::isnan(rsi_14[i])) {
            assert(rsi_14[i] >= 0.0 && rsi_14[i] <= 100.0);
        }
    }
    std::cout << "✓ RSI values are in valid range [0, 100]\n";
}

void test_macd() {
    std::cout << "\n=== Testing MACD ===\n";
    
    auto closes = create_test_closes();
    auto macd_result = QuantLibWrapper::macd(closes, 5, 10, 3); // Shorter periods for test data
    
    print_vector(macd_result.macd_line, "MACD Line", 4);
    print_vector(macd_result.signal_line, "Signal Line", 4);
    print_vector(macd_result.histogram, "Histogram", 4);
    
    // Verify histogram = macd_line - signal_line where both are valid
    for (size_t i = 0; i < macd_result.histogram.size(); ++i) {
        if (!std::isnan(macd_result.macd_line[i]) && !std::isnan(macd_result.signal_line[i])) {
            double expected_histogram = macd_result.macd_line[i] - macd_result.signal_line[i];
            assert(is_close(macd_result.histogram[i], expected_histogram));
        }
    }
    std::cout << "✓ MACD histogram calculation verified\n";
}

void test_bollinger_bands() {
    std::cout << "\n=== Testing Bollinger Bands ===\n";
    
    auto closes = create_test_closes();
    auto bb = QuantLibWrapper::bollinger_bands(closes, 10, 2.0);
    
    print_vector(bb.upper, "Upper Band", 2);
    print_vector(bb.middle, "Middle Band (SMA)", 2);
    print_vector(bb.lower, "Lower Band", 2);
    
    // Verify band relationships where all values are valid
    for (size_t i = 10; i < bb.upper.size(); ++i) {
        if (!std::isnan(bb.upper[i]) && !std::isnan(bb.middle[i]) && !std::isnan(bb.lower[i])) {
            assert(bb.upper[i] >= bb.middle[i]);
            assert(bb.middle[i] >= bb.lower[i]);
        }
    }
    std::cout << "✓ Bollinger Bands ordering verified (Upper >= Middle >= Lower)\n";
}

void test_stochastic() {
    std::cout << "\n=== Testing Stochastic ===\n";
    
    auto highs = create_test_highs();
    auto lows = create_test_lows();
    auto closes = create_test_closes();
    
    auto stoch = QuantLibWrapper::stochastic(highs, lows, closes, 5, 3);
    
    print_vector(stoch.k_percent, "%K", 2);
    print_vector(stoch.d_percent, "%D", 2);
    
    // Stochastic should be between 0 and 100
    for (size_t i = 5; i < stoch.k_percent.size(); ++i) {
        if (!std::isnan(stoch.k_percent[i])) {
            assert(stoch.k_percent[i] >= 0.0 && stoch.k_percent[i] <= 100.0);
        }
    }
    std::cout << "✓ Stochastic %K values are in valid range [0, 100]\n";
}

void test_atr() {
    std::cout << "\n=== Testing ATR ===\n";
    
    auto highs = create_test_highs();
    auto lows = create_test_lows();
    auto closes = create_test_closes();
    
    auto atr = QuantLibWrapper::atr(highs, lows, closes, 5);
    print_vector(atr, "ATR(5)", 2);
    
    // ATR should be positive
    for (size_t i = 5; i < atr.size(); ++i) {
        if (!std::isnan(atr[i])) {
            assert(atr[i] >= 0.0);
        }
    }
    std::cout << "✓ ATR values are non-negative\n";
}

void test_obv() {
    std::cout << "\n=== Testing OBV ===\n";
    
    auto closes = create_test_closes();
    auto volumes = create_test_volumes();
    
    auto obv = QuantLibWrapper::obv(closes, volumes);
    print_vector(obv, "OBV", 0);
    
    // OBV should start with first volume
    assert(is_close(obv[0], volumes[0]));
    std::cout << "✓ OBV calculation verified\n";
}

void test_williams_r() {
    std::cout << "\n=== Testing Williams %R ===\n";
    
    auto highs = create_test_highs();
    auto lows = create_test_lows();
    auto closes = create_test_closes();
    
    auto williams = QuantLibWrapper::williams_r(highs, lows, closes, 5);
    print_vector(williams, "Williams %R", 2);
    
    // Williams %R should be between -100 and 0
    for (size_t i = 5; i < williams.size(); ++i) {
        if (!std::isnan(williams[i])) {
            assert(williams[i] >= -100.0 && williams[i] <= 0.0);
        }
    }
    std::cout << "✓ Williams %R values are in valid range [-100, 0]\n";
}

void test_statistical_functions() {
    std::cout << "\n=== Testing Statistical Functions ===\n";
    
    auto closes = create_test_closes();
    auto volumes = create_test_volumes();
    
    // Test correlation
    double corr = QuantLibWrapper::correlation(closes, volumes);
    std::cout << "Correlation between closes and volumes: " << std::fixed << std::setprecision(4) << corr << "\n";
    assert(corr >= -1.0 && corr <= 1.0);
    std::cout << "✓ Correlation is in valid range [-1, 1]\n";
    
    // Test standard deviation
    double std_dev = QuantLibWrapper::standard_deviation(closes);
    std::cout << "Standard deviation of closes: " << std::fixed << std::setprecision(2) << std_dev << "\n";
    assert(std_dev >= 0.0);
    std::cout << "✓ Standard deviation is non-negative\n";
    
    // Test variance
    double var = QuantLibWrapper::variance(closes);
    std::cout << "Variance of closes: " << std::fixed << std::setprecision(2) << var << "\n";
    assert(var >= 0.0);
    assert(is_close(std_dev * std_dev, var)); // variance = std_dev^2
    std::cout << "✓ Variance calculation verified\n";
}

void test_utility_functions() {
    std::cout << "\n=== Testing Utility Functions ===\n";
    
    auto highs = create_test_highs();
    auto lows = create_test_lows();
    auto closes = create_test_closes();
    
    // Test typical price
    auto typical = QuantLibWrapper::typical_price(highs, lows, closes);
    print_vector(typical, "Typical Price", 2);
    
    // Verify typical price calculation
    double expected_typical = (highs[0] + lows[0] + closes[0]) / 3.0;
    assert(is_close(typical[0], expected_typical));
    std::cout << "✓ Typical price calculation verified\n";
    
    // Test true range
    auto tr = QuantLibWrapper::true_range(highs, lows, closes);
    print_vector(tr, "True Range", 2);
    
    // True range should be non-negative
    for (double value : tr) {
        assert(value >= 0.0);
    }
    std::cout << "✓ True range values are non-negative\n";
}

void test_error_handling() {
    std::cout << "\n=== Testing Error Handling ===\n";
    
    // Test empty data
    std::vector<double> empty_data;
    bool caught_exception = false;
    
    try {
        QuantLibWrapper::sma(empty_data, 5);
    } catch (const std::invalid_argument&) {
        caught_exception = true;
    }
    assert(caught_exception);
    std::cout << "✓ Empty data exception handling works\n";
    
    // Test mismatched data sizes
    std::vector<double> short_data = {1.0, 2.0};
    std::vector<double> long_data = {1.0, 2.0, 3.0, 4.0};
    
    caught_exception = false;
    try {
        QuantLibWrapper::correlation(short_data, long_data);
    } catch (const std::invalid_argument&) {
        caught_exception = true;
    }
    assert(caught_exception);
    std::cout << "✓ Mismatched data size exception handling works\n";
}

int main() {
    std::cout << "Running QuantLib Wrapper Tests...\n";
    std::cout << "====================================\n";
    
    try {
        test_moving_averages();
        test_rsi();
        test_macd();
        test_bollinger_bands();
        test_stochastic();
        test_atr();
        test_obv();
        test_williams_r();
        test_statistical_functions();
        test_utility_functions();
        test_error_handling();
        
        std::cout << "\n====================================\n";
        std::cout << "✅ ALL TESTS PASSED! ✅\n";
        std::cout << "QuantLib wrapper is ready for integration with your MarketDataManager!\n";
        
    } catch (const std::exception& e) {
        std::cout << "\n❌ TEST FAILED: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

// Example integration with your existing MarketDataManager:
/*
void example_integration() {
    // Assuming you have your MarketDataManager set up:
    MarketDataManager market_data;
    
    // Get data from your existing system
    auto recent_closes = market_data.get_recent_closes("BTCUSDT", 100);
    auto recent_highs = market_data.get_recent_highs("BTCUSDT", 100);
    auto recent_lows = market_data.get_recent_lows("BTCUSDT", 100);
    auto recent_volumes = market_data.get_recent_volumes("BTCUSDT", 100);
    
    // Use QuantLib indicators seamlessly
    auto rsi = QuantLibWrapper::rsi(recent_closes, 14);
    auto bb = QuantLibWrapper::bollinger_bands(recent_closes, 20, 2.0);
    auto macd = QuantLibWrapper::macd(recent_closes, 12, 26, 9);
    auto atr = QuantLibWrapper::atr(recent_highs, recent_lows, recent_closes, 14);
    
    // Make trading decisions based on indicators
    double current_rsi = rsi.back();
    double current_price = recent_closes.back();
    double upper_band = bb.upper.back();
    double lower_band = bb.lower.back();
    
    if (current_rsi > 70 && current_price > upper_band) {
        // Potential sell signal
    } else if (current_rsi < 30 && current_price < lower_band) {
        // Potential buy signal
    }
}
*/