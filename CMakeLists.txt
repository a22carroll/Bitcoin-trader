cmake_minimum_required(VERSION 3.15)

# Force vcpkg toolchain before anything else
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

# Set triplet for static linking
set(VCPKG_TARGET_TRIPLET "x64-windows-static")

project(BinanceTest)

# Debug: Print vcpkg status
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")

# Find packages
find_package(CURL REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(QuantLib REQUIRED)

# Debug: Print what jsoncpp provides
get_target_property(JSONCPP_LIBS jsoncpp_lib INTERFACE_LINK_LIBRARIES)
message(STATUS "jsoncpp targets available")
message(STATUS "QuantLib found: ${QuantLib_FOUND}")

# Create test executable for binance
add_executable(test_binance test/test_binance.cpp src/binance.cpp)
target_include_directories(test_binance PRIVATE src)
target_link_libraries(test_binance 
    PRIVATE 
    CURL::libcurl 
    jsoncpp_lib
    OpenSSL::SSL 
    OpenSSL::Crypto
    QuantLib::QuantLib
)
set_property(TARGET test_binance PROPERTY CXX_STANDARD 17)

# Create test executable for market data
add_executable(test_market_data test/test_market_data.cpp src/market_data.cpp)
target_include_directories(test_market_data PRIVATE src)
target_link_libraries(test_market_data 
    PRIVATE 
    QuantLib::QuantLib
)
set_property(TARGET test_market_data PROPERTY CXX_STANDARD 17)

# Main trading bot executable is commented out until main.cpp is created
# add_executable(bitcoin_trader 
#     src/main.cpp 
#     src/binance.cpp 
#     src/market_data.cpp
#     # Add future modules here
# )
# target_include_directories(bitcoin_trader PRIVATE src)
# target_link_libraries(bitcoin_trader 
#     PRIVATE 
#     CURL::libcurl 
#     jsoncpp_lib
#     OpenSSL::SSL 
#     OpenSSL::Crypto
#     QuantLib::QuantLib
# )
# set_property(TARGET bitcoin_trader PROPERTY CXX_STANDARD 17)