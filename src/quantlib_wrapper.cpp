#include "quantlib_wrapper.h"
#include <ql/quantlib.hpp>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace BitcoinTrader {

    // Moving Averages
    std::vector<double> QuantLibWrapper::sma(const std::vector<double>& data, int period) {
        if (!validate_data(data, period)) {
            throw std::invalid_argument("Invalid data or period for SMA calculation");
        }

        std::vector<double> result(data.size(), std::numeric_limits<double>::quiet_NaN());
        
        for (size_t i = period - 1; i < data.size(); ++i) {
            double sum = 0.0;
            for (int j = 0; j < period; ++j) {
                sum += data[i - j];
            }
            result[i] = sum / period;
        }
        
        return result;
    }

    std::vector<double> QuantLibWrapper::ema(const std::vector<double>& data, int period) {
        if (!validate_data(data, period)) {
            throw std::invalid_argument("Invalid data or period for EMA calculation");
        }

        std::vector<double> result(data.size(), std::numeric_limits<double>::quiet_NaN());
        double multiplier = 2.0 / (period + 1.0);
        
        // First EMA value is SMA of first 'period' values
        double sum = 0.0;
        for (int i = 0; i < period; ++i) {
            sum += data[i];
        }
        result[period - 1] = sum / period;
        
        // Calculate EMA for remaining values
        for (size_t i = period; i < data.size(); ++i) {
            result[i] = (data[i] * multiplier) + (result[i - 1] * (1.0 - multiplier));
        }
        
        return result;
    }

    std::vector<double> QuantLibWrapper::wma(const std::vector<double>& data, int period) {
        if (!validate_data(data, period)) {
            throw std::invalid_argument("Invalid data or period for WMA calculation");
        }

        std::vector<double> result(data.size(), std::numeric_limits<double>::quiet_NaN());
        int weight_sum = period * (period + 1) / 2;
        
        for (size_t i = period - 1; i < data.size(); ++i) {
            double weighted_sum = 0.0;
            for (int j = 0; j < period; ++j) {
                weighted_sum += data[i - j] * (period - j);
            }
            result[i] = weighted_sum / weight_sum;
        }
        
        return result;
    }

    // RSI
    std::vector<double> QuantLibWrapper::rsi(const std::vector<double>& data, int period) {
        if (!validate_data(data, period + 1)) {
            throw std::invalid_argument("Invalid data or period for RSI calculation");
        }

        std::vector<double> result(data.size(), std::numeric_limits<double>::quiet_NaN());
        std::vector<double> gains, losses;
        
        // Calculate price changes
        for (size_t i = 1; i < data.size(); ++i) {
            double change = data[i] - data[i - 1];
            gains.push_back(change > 0 ? change : 0.0);
            losses.push_back(change < 0 ? -change : 0.0);
        }
        
        if (gains.size() < static_cast<size_t>(period)) return result;
        
        // Calculate initial averages
        double avg_gain = std::accumulate(gains.begin(), gains.begin() + period, 0.0) / period;
        double avg_loss = std::accumulate(losses.begin(), losses.begin() + period, 0.0) / period;
        
        // Calculate RSI
        for (size_t i = period; i < gains.size(); ++i) {
            if (avg_loss == 0.0) {
                result[i + 1] = 100.0;
            } else {
                double rs = avg_gain / avg_loss;
                result[i + 1] = 100.0 - (100.0 / (1.0 + rs));
            }
            
            // Update averages (Wilder's smoothing)
            avg_gain = ((avg_gain * (period - 1)) + gains[i]) / period;
            avg_loss = ((avg_loss * (period - 1)) + losses[i]) / period;
        }
        
        return result;
    }

    // MACD
    MACD QuantLibWrapper::macd(const std::vector<double>& data, int fast_period, 
                              int slow_period, int signal_period) {
        if (!validate_data(data, slow_period + signal_period)) {
            throw std::invalid_argument("Invalid data for MACD calculation");
        }

        auto fast_ema = ema(data, fast_period);
        auto slow_ema = ema(data, slow_period);
        
        MACD result(data.size());
        
        // Calculate MACD line
        for (size_t i = 0; i < data.size(); ++i) {
            if (!std::isnan(fast_ema[i]) && !std::isnan(slow_ema[i])) {
                result.macd_line[i] = fast_ema[i] - slow_ema[i];
            } else {
                result.macd_line[i] = std::numeric_limits<double>::quiet_NaN();
            }
        }
        
        // Calculate signal line (EMA of MACD line)
        result.signal_line = ema(result.macd_line, signal_period);
        
        // Calculate histogram
        for (size_t i = 0; i < data.size(); ++i) {
            if (!std::isnan(result.macd_line[i]) && !std::isnan(result.signal_line[i])) {
                result.histogram[i] = result.macd_line[i] - result.signal_line[i];
            } else {
                result.histogram[i] = std::numeric_limits<double>::quiet_NaN();
            }
        }
        
        return result;
    }

    // Stochastic
    Stochastic QuantLibWrapper::stochastic(const std::vector<double>& highs,
                                         const std::vector<double>& lows,
                                         const std::vector<double>& closes,
                                         int k_period, int d_period) {
        if (!validate_hlc_data(highs, lows, closes) || 
            highs.size() < static_cast<size_t>(k_period + d_period)) {
            throw std::invalid_argument("Invalid data for Stochastic calculation");
        }

        Stochastic result(highs.size());
        
        // Calculate %K
        for (size_t i = k_period - 1; i < highs.size(); ++i) {
            double highest = *std::max_element(highs.begin() + i - k_period + 1, highs.begin() + i + 1);
            double lowest = *std::min_element(lows.begin() + i - k_period + 1, lows.begin() + i + 1);
            
            if (highest != lowest) {
                result.k_percent[i] = ((closes[i] - lowest) / (highest - lowest)) * 100.0;
            } else {
                result.k_percent[i] = 50.0; // Neutral when no range
            }
        }
        
        // Calculate %D (SMA of %K)
        result.d_percent = sma(result.k_percent, d_period);
        
        return result;
    }

    // Bollinger Bands
    BollingerBands QuantLibWrapper::bollinger_bands(const std::vector<double>& data, 
                                                   int period, double std_dev) {
        if (!validate_data(data, period)) {
            throw std::invalid_argument("Invalid data or period for Bollinger Bands calculation");
        }

        BollingerBands result(data.size());
        
        // Calculate middle band (SMA)
        result.middle = sma(data, period);
        
        // Calculate standard deviation and bands
        for (size_t i = period - 1; i < data.size(); ++i) {
            // Calculate standard deviation for the period
            double mean = result.middle[i];
            double variance = 0.0;
            
            for (int j = 0; j < period; ++j) {
                double diff = data[i - j] - mean;
                variance += diff * diff;
            }
            variance /= period;
            double stdev = std::sqrt(variance);
            
            result.upper[i] = mean + (std_dev * stdev);
            result.lower[i] = mean - (std_dev * stdev);
        }
        
        return result;
    }

    // ATR
    std::vector<double> QuantLibWrapper::atr(const std::vector<double>& highs,
                                           const std::vector<double>& lows,
                                           const std::vector<double>& closes,
                                           int period) {
        if (!validate_hlc_data(highs, lows, closes) || 
            highs.size() < static_cast<size_t>(period + 1)) {
            throw std::invalid_argument("Invalid data for ATR calculation");
        }

        auto tr = true_range(highs, lows, closes);
        return sma(tr, period);
    }

    // On-Balance Volume
    std::vector<double> QuantLibWrapper::obv(const std::vector<double>& closes,
                                           const std::vector<double>& volumes) {
        if (closes.size() != volumes.size() || closes.size() < 2) {
            throw std::invalid_argument("Invalid data for OBV calculation");
        }

        std::vector<double> result(closes.size());
        result[0] = volumes[0];
        
        for (size_t i = 1; i < closes.size(); ++i) {
            if (closes[i] > closes[i - 1]) {
                result[i] = result[i - 1] + volumes[i];
            } else if (closes[i] < closes[i - 1]) {
                result[i] = result[i - 1] - volumes[i];
            } else {
                result[i] = result[i - 1];
            }
        }
        
        return result;
    }

    // Williams %R
    std::vector<double> QuantLibWrapper::williams_r(const std::vector<double>& highs,
                                                  const std::vector<double>& lows,
                                                  const std::vector<double>& closes,
                                                  int period) {
        if (!validate_hlc_data(highs, lows, closes) || 
            highs.size() < static_cast<size_t>(period)) {
            throw std::invalid_argument("Invalid data for Williams %R calculation");
        }

        std::vector<double> result(highs.size(), std::numeric_limits<double>::quiet_NaN());
        
        for (size_t i = period - 1; i < highs.size(); ++i) {
            double highest = *std::max_element(highs.begin() + i - period + 1, highs.begin() + i + 1);
            double lowest = *std::min_element(lows.begin() + i - period + 1, lows.begin() + i + 1);
            
            if (highest != lowest) {
                result[i] = ((highest - closes[i]) / (highest - lowest)) * -100.0;
            } else {
                result[i] = -50.0; // Neutral when no range
            }
        }
        
        return result;
    }

    // Statistical Functions
    double QuantLibWrapper::correlation(const std::vector<double>& x, const std::vector<double>& y) {
        if (x.size() != y.size() || x.size() < 2) {
            throw std::invalid_argument("Invalid data for correlation calculation");
        }

        double mean_x = std::accumulate(x.begin(), x.end(), 0.0) / x.size();
        double mean_y = std::accumulate(y.begin(), y.end(), 0.0) / y.size();
        
        double sum_xy = 0.0, sum_x2 = 0.0, sum_y2 = 0.0;
        
        for (size_t i = 0; i < x.size(); ++i) {
            double dx = x[i] - mean_x;
            double dy = y[i] - mean_y;
            sum_xy += dx * dy;
            sum_x2 += dx * dx;
            sum_y2 += dy * dy;
        }
        
        if (sum_x2 == 0.0 || sum_y2 == 0.0) return 0.0;
        
        return sum_xy / std::sqrt(sum_x2 * sum_y2);
    }

    double QuantLibWrapper::standard_deviation(const std::vector<double>& data) {
        return std::sqrt(variance(data));
    }

    double QuantLibWrapper::variance(const std::vector<double>& data) {
        if (data.size() < 2) {
            throw std::invalid_argument("Need at least 2 data points for variance calculation");
        }

        double mean = std::accumulate(data.begin(), data.end(), 0.0) / data.size();
        double variance = 0.0;
        
        for (double value : data) {
            double diff = value - mean;
            variance += diff * diff;
        }
        
        return variance / (data.size() - 1); // Sample variance
    }

    // Utility Functions
    std::vector<double> QuantLibWrapper::typical_price(const std::vector<double>& highs,
                                                     const std::vector<double>& lows,
                                                     const std::vector<double>& closes) {
        if (!validate_hlc_data(highs, lows, closes)) {
            throw std::invalid_argument("Invalid HLC data for typical price calculation");
        }

        std::vector<double> result(highs.size());
        
        for (size_t i = 0; i < highs.size(); ++i) {
            result[i] = (highs[i] + lows[i] + closes[i]) / 3.0;
        }
        
        return result;
    }

    std::vector<double> QuantLibWrapper::true_range(const std::vector<double>& highs,
                                                  const std::vector<double>& lows,
                                                  const std::vector<double>& closes) {
        if (!validate_hlc_data(highs, lows, closes)) {
            throw std::invalid_argument("Invalid HLC data for true range calculation");
        }

        std::vector<double> result(highs.size());
        result[0] = highs[0] - lows[0]; // First value
        
        for (size_t i = 1; i < highs.size(); ++i) {
            double tr1 = highs[i] - lows[i];
            double tr2 = std::abs(highs[i] - closes[i - 1]);
            double tr3 = std::abs(lows[i] - closes[i - 1]);
            
            result[i] = std::max({tr1, tr2, tr3});
        }
        
        return result;
    }

    // Helper Functions
    std::vector<double> QuantLibWrapper::pad_with_nan(size_t total_size, size_t valid_start) {
        std::vector<double> result(total_size, std::numeric_limits<double>::quiet_NaN());
        return result;
    }

    bool QuantLibWrapper::validate_data(const std::vector<double>& data, int min_size) {
        return !data.empty() && static_cast<int>(data.size()) >= min_size;
    }

    bool QuantLibWrapper::validate_hlc_data(const std::vector<double>& highs,
                                          const std::vector<double>& lows,
                                          const std::vector<double>& closes) {
        return highs.size() == lows.size() && 
               lows.size() == closes.size() && 
               !highs.empty();
    }

} // namespace BitcoinTrader