#pragma once

#include <vector>
#include <string>
#include <memory>

namespace BitcoinTrader {

    // Structure for Bollinger Bands result
    struct BollingerBands {
        std::vector<double> upper;
        std::vector<double> middle;  // SMA
        std::vector<double> lower;
        
        BollingerBands(size_t size) : upper(size), middle(size), lower(size) {}
    };

    // Structure for MACD result
    struct MACD {
        std::vector<double> macd_line;
        std::vector<double> signal_line;
        std::vector<double> histogram;
        
        MACD(size_t size) : macd_line(size), signal_line(size), histogram(size) {}
    };

    // Structure for Stochastic result
    struct Stochastic {
        std::vector<double> k_percent;
        std::vector<double> d_percent;
        
        Stochastic(size_t size) : k_percent(size), d_percent(size) {}
    };

    /**
     * @brief QuantLib wrapper class providing clean interface to financial indicators
     * 
     * This class wraps QuantLib's financial indicators to provide a simple interface
     * that works seamlessly with the existing TimeSeries data from MarketDataManager.
     */
    class QuantLibWrapper {
    public:
        // Moving Averages
        static std::vector<double> sma(const std::vector<double>& data, int period);
        static std::vector<double> ema(const std::vector<double>& data, int period);
        static std::vector<double> wma(const std::vector<double>& data, int period);

        // Oscillators
        static std::vector<double> rsi(const std::vector<double>& data, int period = 14);
        static MACD macd(const std::vector<double>& data, int fast_period = 12, 
                        int slow_period = 26, int signal_period = 9);
        static Stochastic stochastic(const std::vector<double>& highs,
                                   const std::vector<double>& lows,
                                   const std::vector<double>& closes,
                                   int k_period = 14, int d_period = 3);

        // Bands and Channels
        static BollingerBands bollinger_bands(const std::vector<double>& data, 
                                             int period = 20, double std_dev = 2.0);

        // Volatility Indicators
        static std::vector<double> atr(const std::vector<double>& highs,
                                     const std::vector<double>& lows,
                                     const std::vector<double>& closes,
                                     int period = 14);

        // Volume Indicators
        static std::vector<double> obv(const std::vector<double>& closes,
                                     const std::vector<double>& volumes);

        // Trend Indicators
        static std::vector<double> adx(const std::vector<double>& highs,
                                     const std::vector<double>& lows,
                                     const std::vector<double>& closes,
                                     int period = 14);

        // Momentum Indicators
        static std::vector<double> cci(const std::vector<double>& highs,
                                     const std::vector<double>& lows,
                                     const std::vector<double>& closes,
                                     int period = 20);
        
        static std::vector<double> williams_r(const std::vector<double>& highs,
                                            const std::vector<double>& lows,
                                            const std::vector<double>& closes,
                                            int period = 14);

        // Statistical Functions
        static double correlation(const std::vector<double>& x, const std::vector<double>& y);
        static double standard_deviation(const std::vector<double>& data);
        static double variance(const std::vector<double>& data);

        // Utility Functions
        static std::vector<double> typical_price(const std::vector<double>& highs,
                                               const std::vector<double>& lows,
                                               const std::vector<double>& closes);

        static std::vector<double> true_range(const std::vector<double>& highs,
                                            const std::vector<double>& lows,
                                            const std::vector<double>& closes);

    private:
        // Helper functions
        static std::vector<double> pad_with_nan(size_t total_size, size_t valid_start);
        static bool validate_data(const std::vector<double>& data, int min_size = 1);
        static bool validate_hlc_data(const std::vector<double>& highs,
                                    const std::vector<double>& lows,
                                    const std::vector<double>& closes);
    };

} // namespace BitcoinTrader