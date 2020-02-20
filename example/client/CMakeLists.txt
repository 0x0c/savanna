cmake_minimum_required(VERSION 3.15.0)

project(simple_client)
set(CMAKE_CXX_STANDARD 11)

add_executable(simple_client main.cpp)

if (APPLE)
find_package(openssl REQUIRED)
else()
find_package(OpenSSL REQUIRED)
endif()

target_link_libraries(simple_client
    savanna
    OpenSSL::SSL
    OpenSSL::Crypto)

target_include_directories(simple_client INTERFACE ../../include)
