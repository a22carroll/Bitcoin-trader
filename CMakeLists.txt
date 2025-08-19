cmake_minimum_required(VERSION 3.15)

# Force vcpkg toolchain before anything else
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

project(BinanceTest)

# Debug: Print vcpkg status
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")

# Find packages
find_package(CURL REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# Debug: Print what jsoncpp provides
get_target_property(JSONCPP_LIBS jsoncpp_lib INTERFACE_LINK_LIBRARIES)
message(STATUS "jsoncpp targets available")

# Create test executable - test_binance.cpp is in test/, binance.cpp is in src/
add_executable(test_binance test/test_binance.cpp src/binance.cpp)

# Tell the compiler where to find binance.h
target_include_directories(test_binance PRIVATE src)

# Link libraries - try the most common jsoncpp target name
target_link_libraries(test_binance 
    PRIVATE 
    CURL::libcurl 
    jsoncpp_lib
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# Set C++ standard
set_property(TARGET test_binance PROPERTY CXX_STANDARD 17)