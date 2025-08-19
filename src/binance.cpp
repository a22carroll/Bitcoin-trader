#include "binance.h"
#include <curl/curl.h>
#include <json/json.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <chrono>
#include <fstream>
#include <filesystem>

namespace binance {

// Callback function for libcurl to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    size_t total_size = size * nmemb;
    data->append((char*)contents, total_size);
    return total_size;
}

// Constructor
BinanceClient::BinanceClient(bool testnet) : testnet_(testnet) {
    if (testnet_) {
        base_url_ = "https://testnet.binance.vision";
    } else {
        base_url_ = "https://api.binance.com";
    }
    
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Try to load config from file
    load_config_from_file("config.json");
}

// Load configuration from JSON file
void BinanceClient::load_config_from_file(const std::string& config_file) {
    // Add these debug lines
    std::cout << "Looking for config file: " << config_file << std::endl;
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: " << config_file << std::endl;
        return;
    }
    
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(file, root)) {
        std::cerr << "Warning: Could not parse config file: " << config_file << std::endl;
        return;
    }
    
    if (root.isMember("binance")) {
        Json::Value binance_config = root["binance"];
        
        if (binance_config.isMember("api_key")) {
            api_key_ = binance_config["api_key"].asString();
        }
        
        if (binance_config.isMember("secret_key")) {
            secret_key_ = binance_config["secret_key"].asString();
        }
        
        if (binance_config.isMember("testnet")) {
            testnet_ = binance_config["testnet"].asBool();
            if (testnet_) {
                base_url_ = "https://testnet.binance.vision";
            } else {
                base_url_ = "https://api.binance.com";
            }
        }
        
        if (binance_config.isMember("base_url")) {
            base_url_ = binance_config["base_url"].asString();
        }
    }
}

// Set API credentials manually
void BinanceClient::set_credentials(const std::string& api_key, const std::string& secret_key) {
    api_key_ = api_key;
    secret_key_ = secret_key;
}

// Create HMAC SHA256 signature
std::string BinanceClient::create_signature(const std::string& data) const {
    unsigned char* digest = HMAC(EVP_sha256(), 
                                secret_key_.c_str(), secret_key_.length(),
                                (unsigned char*)data.c_str(), data.length(), 
                                NULL, NULL);
    
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)digest[i];
    }
    return ss.str();
}

// Get current timestamp in milliseconds
long BinanceClient::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return timestamp.count();
}

// Make HTTP request to public endpoints
std::string BinanceClient::make_request(const std::string& endpoint) const {
    CURL* curl;
    CURLcode res;
    std::string response_data;
    
    curl = curl_easy_init();
    if (curl) {
        std::string url = base_url_ + endpoint;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return "";
        }
    }
    
    return response_data;
}

// Make authenticated HTTP request to private endpoints
std::string BinanceClient::make_signed_request(const std::string& endpoint) const {
    if (api_key_.empty() || secret_key_.empty()) {
        std::cerr << "Error: API credentials not set for signed request" << std::endl;
        return "";
    }
    
    CURL* curl;
    CURLcode res;
    std::string response_data;
    
    curl = curl_easy_init();
    if (curl) {
        // Add timestamp to query
        std::string query = "timestamp=" + std::to_string(get_timestamp());
        
        // Create signature
        std::string signature = create_signature(query);
        query += "&signature=" + signature;
        
        // Build full URL
        std::string url = base_url_ + endpoint;
        if (endpoint.find('?') != std::string::npos) {
            url += "&" + query;
        } else {
            url += "?" + query;
        }
        
        // Set headers
        struct curl_slist* headers = NULL;
        std::string auth_header = "X-MBX-APIKEY: " + api_key_;
        headers = curl_slist_append(headers, auth_header.c_str());
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return "";
        }
    }
    
    return response_data;
}

// Get current price for a symbol
double BinanceClient::get_price(const std::string& symbol) const {
    std::string endpoint = "/api/v3/ticker/price?symbol=" + symbol;
    std::string response = make_request(endpoint);
    
    if (response.empty()) {
        return 0.0;
    }
    
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(response, root)) {
        std::cerr << "Error parsing price response" << std::endl;
        return 0.0;
    }
    
    if (root.isMember("price")) {
        return std::stod(root["price"].asString());
    }
    
    return 0.0;
}

// Get historical kline data
std::vector<Kline> BinanceClient::get_klines(const std::string& symbol, 
                                            const std::string& interval, 
                                            int limit) const {
    std::vector<Kline> klines;
    
    std::string endpoint = "/api/v3/klines?symbol=" + symbol + 
                          "&interval=" + interval + 
                          "&limit=" + std::to_string(limit);
    
    std::string response = make_request(endpoint);
    
    if (response.empty()) {
        return klines;
    }
    
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(response, root)) {
        std::cerr << "Error parsing klines response" << std::endl;
        return klines;
    }
    
    if (root.isArray()) {
        for (const auto& kline_data : root) {
            if (kline_data.isArray() && kline_data.size() >= 6) {
                Kline kline;
                kline.timestamp = kline_data[0].asInt64();
                kline.open = std::stod(kline_data[1].asString());
                kline.high = std::stod(kline_data[2].asString());
                kline.low = std::stod(kline_data[3].asString());
                kline.close = std::stod(kline_data[4].asString());
                kline.volume = std::stod(kline_data[5].asString());
                
                klines.push_back(kline);
            }
        }
    }
    
    return klines;
}

// Get account balances
std::vector<Balance> BinanceClient::get_balances() const {
    std::vector<Balance> balances;
    
    std::string response = make_signed_request("/api/v3/account");
    
    if (response.empty()) {
        return balances;
    }
    
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(response, root)) {
        std::cerr << "Error parsing account response" << std::endl;
        return balances;
    }
    
    if (root.isMember("balances") && root["balances"].isArray()) {
        for (const auto& balance_data : root["balances"]) {
            Balance balance;
            balance.asset = balance_data["asset"].asString();
            balance.free = std::stod(balance_data["free"].asString());
            balance.locked = std::stod(balance_data["locked"].asString());
            
            // Only include balances with non-zero amounts
            if (balance.free > 0.0 || balance.locked > 0.0) {
                balances.push_back(balance);
            }
        }
    }
    
    return balances;
}

// Get balance for specific asset
double BinanceClient::get_balance(const std::string& asset) const {
    auto balances = get_balances();
    
    for (const auto& balance : balances) {
        if (balance.asset == asset) {
            return balance.free + balance.locked;
        }
    }
    
    return 0.0;
}

// Test connection to Binance API
bool BinanceClient::test_connection() const {
    std::string response = make_request("/api/v3/ping");
    return !response.empty();
}

} // namespace binance