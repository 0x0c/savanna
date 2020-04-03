cmake_minimum_required(VERSION 3.15.0)

project(simple_ws)
set(CMAKE_CXX_STANDARD 11)

find_package(OpenSSL REQUIRED)
if (OpenSSL_FOUND)
    add_executable(simple_ws main.cpp)
    target_link_libraries(simple_ws
        savanna
        ssl
        crypto)
    target_include_directories(simple_ws INTERFACE ../../include)
endif()