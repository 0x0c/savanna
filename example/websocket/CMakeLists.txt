cmake_minimum_required(VERSION 3.15.2)

project(simple_ws)
set(CMAKE_CXX_STANDARD 11)

add_executable(simple_ws main.cpp)

find_package(openssl REQUIRED)
target_link_libraries(simple_ws
    savanna
    OpenSSL::SSL
    OpenSSL::Crypto)

target_include_directories(simple_ws INTERFACE ../../include)