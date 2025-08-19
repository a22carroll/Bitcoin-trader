#include "market_data.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <numeric>

namespace BitcoinTrader {

// TimeSeries Implementation
TimeSeries::TimeSeries(size_t max_size, bool auto_trim)
    : max_size_(max_size), auto_trim_(auto_trim) {
    if (max_size == 0) {
        throw std::invalid_argument("Maximum size must be greater than 0");
    }
}

TimeSeries::TimeSeries(const std::vector<Kline>& initial_data, size_t max_size, bool auto_trim)
    : max_size_(max_size), auto_trim_(auto_trim) {
    if (max_size == 0) {
        throw std::invalid_argument("Maximum size must be greater than 0");
    }
    add_klines(initial_data);
}

void TimeSeries::add_kline(const Kline& kline) {
    if (!kline.is_valid()) {
        std::cerr << "Warning: Adding invalid kline with timestamp " << kline.timestamp << std::endl;
    }
    
    data_.push_back(kline);
    
    if (auto_trim_ && data_.size() > max_size_) {
        data_.pop_front();
    }
}

void TimeSeries::add_klines(const std::vector<Kline>& klines) {
    for (const auto& kline : klines) {
        add_kline(kline);
    }
}

void TimeSeries::clear() {
    data_.clear();
}

void TimeSeries::reserve(size_t size) {
    // Note: std::deque doesn't have reserve, but we can prepare for the size
    if (size > max_size_) {
        max_size_ = size;
    }
}

const Kline& TimeSeries::operator[](size_t index) const {
    return data_[index];
}

const Kline& TimeSeries::at(size_t index) const {
    if (index >= data_.size()) {
        throw std::out_of_range("Index out of range");
    }
    return data_[index];
}

const Kline& TimeSeries::latest() const {
    if (data_.empty()) {
        throw std::runtime_error("No data available");
    }
    return data_.back();
}

const Kline& TimeSeries::oldest() const {
    if (data_.empty()) {
        throw std::runtime_error("No data available");
    }
    return data_.front();
}

std::vector<double> TimeSeries::get_closes(size_t count) const {
    if (count == 0) count = data_.size();
    count = std::min(count, data_.size());
    
    std::vector<double> closes;
    closes.reserve(count);
    
    size_t start_index = data_.size() - count;
    for (size_t i = start_index; i < data_.size(); ++i) {
        closes.push_back(data_[i].close);
    }
    
    return closes;
}

std::vector<double> TimeSeries::get_opens(size_t count) const {
    if (count == 0) count = data_.size();
    count = std::min(count, data_.size());
    
    std::vector<double> opens;
    opens.reserve(count);
    
    size_t start_index = data_.size() - count;
    for (size_t i = start_index; i < data_.size(); ++i) {
        opens.push_back(data_[i].open);
    }
    
    return opens;
}

std::vector<double> TimeSeries::get_highs(size_t count) const {
    if (count == 0) count = data_.size();
    count = std::min(count, data_.size());
    
    std::vector<double> highs;
    highs.reserve(count);
    
    size_t start_index = data_.size() - count;
    for (size_t i = start_index; i < data_.size(); ++i) {
        highs.push_back(data_[i].high);
    }
    
    return highs;
}

std::vector<double> TimeSeries::get_lows(size_t count) const {
    if (count == 0) count = data_.size();
    count = std::min(count, data_.size());
    
    std::vector<double> lows;
    lows.reserve(count);
    
    size_t start_index = data_.size() - count;
    for (size_t i = start_index; i < data_.size(); ++i) {
        lows.push_back(data_[i].low);
    }
    
    return lows;
}

std::vector<double> TimeSeries::get_volumes(size_t count) const {
    if (count == 0) count = data_.size();
    count = std::min(count, data_.size());
    
    std::vector<double> volumes;
    volumes.reserve(count);
    
    size_t start_index = data_.size() - count;
    for (size_t i = start_index; i < data_.size(); ++i) {
        volumes.push_back(data_[i].volume);
    }
    
    return volumes;
}

std::vector<double> TimeSeries::get_typical_prices(size_t count) const {
    if (count == 0) count = data_.size();
    count = std::min(count, data_.size());
    
    std::vector<double> typical_prices;
    typical_prices.reserve(count);
    
    size_t start_index = data_.size() - count;
    for (size_t i = start_index; i < data_.size(); ++i) {
        typical_prices.push_back(data_[i].typical_price());
    }
    
    return typical_prices;
}

std::vector<Kline> TimeSeries::get_range(long long start_time, long long end_time) const {
    std::vector<Kline> result;
    
    for (const auto& kline : data_) {
        if (kline.timestamp >= start_time && kline.timestamp <= end_time) {
            result.push_back(kline);
        }
    }
    
    return result;
}

std::vector<Kline> TimeSeries::get_last_n_klines(size_t n) const {
    if (n >= data_.size()) {
        return std::vector<Kline>(data_.begin(), data_.end());
    }
    
    auto start_it = data_.end() - n;
    return std::vector<Kline>(start_it, data_.end());
}

double TimeSeries::get_price_change(size_t periods) const {
    if (data_.size() <= periods) {
        return 0.0;
    }
    
    double current_price = data_.back().close;
    double past_price = data_[data_.size() - 1 - periods].close;
    
    return current_price - past_price;
}

double TimeSeries::get_price_change_percent(size_t periods) const {
    if (data_.size() <= periods) {
        return 0.0;
    }
    
    double current_price = data_.back().close;
    double past_price = data_[data_.size() - 1 - periods].close;
    
    if (past_price == 0.0) {
        return 0.0;
    }
    
    return ((current_price - past_price) / past_price) * 100.0;
}

double TimeSeries::get_average_volume(size_t periods) const {
    if (data_.empty()) {
        return 0.0;
    }
    
    if (periods == 0) periods = data_.size();
    periods = std::min(periods, data_.size());
    
    double sum = 0.0;
    size_t start_index = data_.size() - periods;
    
    for (size_t i = start_index; i < data_.size(); ++i) {
        sum += data_[i].volume;
    }
    
    return sum / periods;
}

double TimeSeries::get_price_volatility(size_t periods) const {
    if (data_.size() < 2) {
        return 0.0;
    }
    
    if (periods == 0) periods = data_.size();
    periods = std::min(periods, data_.size());
    
    std::vector<double> returns;
    size_t start_index = data_.size() - periods;
    
    for (size_t i = start_index; i < data_.size() - 1; ++i) {
        double current_price = data_[i].close;
        double next_price = data_[i + 1].close;
        
        if (current_price > 0) {
            double return_val = (next_price - current_price) / current_price;
            returns.push_back(return_val);
        }
    }
    
    if (returns.empty()) {
        return 0.0;
    }
    
    return MarketDataUtils::calculate_stddev(returns);
}

bool TimeSeries::validate_data() const {
    for (const auto& kline : data_) {
        if (!kline.is_valid()) {
            return false;
        }
    }
    return true;
}

size_t TimeSeries::count_invalid_klines() const {
    size_t count = 0;
    for (const auto& kline : data_) {
        if (!kline.is_valid()) {
            count++;
        }
    }
    return count;
}

void TimeSeries::set_max_size(size_t new_max_size) {
    if (new_max_size == 0) {
        throw std::invalid_argument("Maximum size must be greater than 0");
    }
    
    max_size_ = new_max_size;
    
    if (auto_trim_ && data_.size() > max_size_) {
        size_t excess = data_.size() - max_size_;
        for (size_t i = 0; i < excess; ++i) {
            data_.pop_front();
        }
    }
}

void TimeSeries::set_auto_trim(bool auto_trim) {
    auto_trim_ = auto_trim;
}

// MarketDataManager Implementation
void MarketDataManager::register_symbol(const std::string& symbol, const std::string& interval, size_t max_size) {
    symbol_data_[symbol] = std::make_unique<TimeSeries>(max_size, true);
    symbol_intervals_[symbol] = interval;
}

void MarketDataManager::unregister_symbol(const std::string& symbol) {
    symbol_data_.erase(symbol);
    symbol_intervals_.erase(symbol);
}

bool MarketDataManager::has_symbol(const std::string& symbol) const {
    return symbol_data_.find(symbol) != symbol_data_.end();
}

std::vector<std::string> MarketDataManager::get_registered_symbols() const {
    std::vector<std::string> symbols;
    for (const auto& pair : symbol_data_) {
        symbols.push_back(pair.first);
    }
    return symbols;
}

void MarketDataManager::add_kline(const std::string& symbol, const Kline& kline) {
    auto it = symbol_data_.find(symbol);
    if (it != symbol_data_.end()) {
        it->second->add_kline(kline);
    } else {
        throw std::runtime_error("Symbol " + symbol + " not registered");
    }
}

void MarketDataManager::add_klines(const std::string& symbol, const std::vector<Kline>& klines) {
    auto it = symbol_data_.find(symbol);
    if (it != symbol_data_.end()) {
        it->second->add_klines(klines);
    } else {
        throw std::runtime_error("Symbol " + symbol + " not registered");
    }
}

void MarketDataManager::update_latest_price(const std::string& symbol, double price) {
    auto it = symbol_data_.find(symbol);
    if (it != symbol_data_.end() && !it->second->empty()) {
        // Create a new kline based on the latest one with updated close price
        Kline latest = it->second->latest();
        latest.close = price;
        latest.high = std::max(latest.high, price);
        latest.low = std::min(latest.low, price);
        latest.timestamp = MarketDataUtils::current_timestamp_ms();
        
        // Replace the latest kline
        // Note: This is a simplified approach. In practice, you might want
        // to handle real-time updates differently
        it->second->add_kline(latest);
    }
}

const TimeSeries* MarketDataManager::get_time_series(const std::string& symbol) const {
    auto it = symbol_data_.find(symbol);
    return (it != symbol_data_.end()) ? it->second.get() : nullptr;
}

TimeSeries* MarketDataManager::get_time_series_mutable(const std::string& symbol) {
    auto it = symbol_data_.find(symbol);
    return (it != symbol_data_.end()) ? it->second.get() : nullptr;
}

const Kline& MarketDataManager::get_latest_kline(const std::string& symbol) const {
    auto it = symbol_data_.find(symbol);
    if (it != symbol_data_.end()) {
        return it->second->latest();
    }
    throw std::runtime_error("Symbol " + symbol + " not found or has no data");
}

double MarketDataManager::get_latest_price(const std::string& symbol) const {
    return get_latest_kline(symbol).close;
}

std::vector<double> MarketDataManager::get_recent_closes(const std::string& symbol, size_t count) const {
    auto it = symbol_data_.find(symbol);
    if (it != symbol_data_.end()) {
        return it->second->get_closes(count);
    }
    return std::vector<double>();
}

std::map<std::string, double> MarketDataManager::get_all_latest_prices() const {
    std::map<std::string, double> prices;
    for (const auto& pair : symbol_data_) {
        if (!pair.second->empty()) {
            prices[pair.first] = pair.second->latest().close;
        }
    }
    return prices;
}

std::map<std::string, double> MarketDataManager::get_price_changes_24h() const {
    std::map<std::string, double> changes;
    for (const auto& pair : symbol_data_) {
        if (!pair.second->empty()) {
            // Assuming 24h = 24 * 60 = 1440 minutes for 1m intervals
            // Adjust based on your actual interval
            changes[pair.first] = pair.second->get_price_change_percent(1440);
        }
    }
    return changes;
}

std::map<std::string, size_t> MarketDataManager::get_data_counts() const {
    std::map<std::string, size_t> counts;
    for (const auto& pair : symbol_data_) {
        counts[pair.first] = pair.second->size();
    }
    return counts;
}

void MarketDataManager::clear_all_data() {
    for (auto& pair : symbol_data_) {
        pair.second->clear();
    }
}

void MarketDataManager::clear_symbol_data(const std::string& symbol) {
    auto it = symbol_data_.find(symbol);
    if (it != symbol_data_.end()) {
        it->second->clear();
    }
}

std::string MarketDataManager::get_market_summary() const {
    std::ostringstream oss;
    oss << "Market Data Summary:\n";
    oss << "==================\n";
    
    for (const auto& pair : symbol_data_) {
        const std::string& symbol = pair.first;
        const auto& ts = pair.second;
        
        if (!ts->empty()) {
            const Kline& latest = ts->latest();
            double change_24h = ts->get_price_change_percent(1440);
            
            oss << symbol << " (" << symbol_intervals_.at(symbol) << "): "
                << MarketDataUtils::format_price(latest.close) 
                << " (" << MarketDataUtils::format_percentage(change_24h) << ") "
                << "Data points: " << ts->size() << "\n";
        } else {
            oss << symbol << ": No data\n";
        }
    }
    
    return oss.str();
}

// MarketDataUtils Implementation
namespace MarketDataUtils {

long long current_timestamp_ms() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::string timestamp_to_string(long long timestamp) {
    auto time_point = std::chrono::system_clock::from_time_t(timestamp / 1000);
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

long long string_to_timestamp(const std::string& time_str) {
    // This is a simplified implementation
    // In practice, you'd use a proper time parsing library
    std::tm tm = {};
    std::istringstream ss(time_str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    auto duration = time_point.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

bool is_valid_price(double price) {
    return price > 0.0 && std::isfinite(price);
}

bool is_valid_volume(double volume) {
    return volume >= 0.0 && std::isfinite(volume);
}

bool is_valid_timestamp(long long timestamp) {
    return timestamp > 0;
}

double calculate_sma(const std::vector<double>& prices, size_t period) {
    if (prices.size() < period || period == 0) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (size_t i = prices.size() - period; i < prices.size(); ++i) {
        sum += prices[i];
    }
    
    return sum / period;
}

double calculate_ema(const std::vector<double>& prices, size_t period) {
    if (prices.empty() || period == 0) {
        return 0.0;
    }
    
    if (prices.size() == 1) {
        return prices[0];
    }
    
    double multiplier = 2.0 / (period + 1);
    double ema = prices[0];
    
    for (size_t i = 1; i < prices.size(); ++i) {
        ema = (prices[i] * multiplier) + (ema * (1 - multiplier));
    }
    
    return ema;
}

double calculate_stddev(const std::vector<double>& values) {
    if (values.size() <= 1) {
        return 0.0;
    }
    
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double sq_sum = 0.0;
    
    for (double value : values) {
        sq_sum += (value - mean) * (value - mean);
    }
    
    return std::sqrt(sq_sum / (values.size() - 1));
}

double calculate_variance(const std::vector<double>& values) {
    if (values.size() <= 1) {
        return 0.0;
    }
    
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double sq_sum = 0.0;
    
    for (double value : values) {
        sq_sum += (value - mean) * (value - mean);
    }
    
    return sq_sum / (values.size() - 1);
}

std::string format_price(double price, int decimal_places) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimal_places) << "$" << price;
    return oss.str();
}

std::string format_volume(double volume) {
    std::ostringstream oss;
    if (volume >= 1000000) {
        oss << std::fixed << std::setprecision(2) << (volume / 1000000) << "M";
    } else if (volume >= 1000) {
        oss << std::fixed << std::setprecision(2) << (volume / 1000) << "K";
    } else {
        oss << std::fixed << std::setprecision(2) << volume;
    }
    return oss.str();
}

std::string format_percentage(double percentage) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    if (percentage >= 0) {
        oss << "+";
    }
    oss << percentage << "%";
    return oss.str();
}

} // namespace MarketDataUtils

} // namespace BitcoinTrader