cmake_minimum_required(VERSION 3.15.2)

project(simple_client)
set(CMAKE_CXX_STANDARD 11)

add_executable(simple_client main.cpp)

target_link_libraries(simple_client savanna)

target_include_directories(simple_client INTERFACE ../include)