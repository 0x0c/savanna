cmake_minimum_required(VERSION 3.15.0)

project(simple_async_ws)
set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

find_package(OpenSSL REQUIRED)
if (OpenSSL_FOUND)
    add_executable(simple_async_ws main.cpp)
    target_link_libraries(
        simple_async_ws
        savanna
        ${OPENSSL_CRYPTO_LIBRARY}
        ${OPENSSL_SSL_LIBRARY}
        Threads::Threads
    )
    target_include_directories(simple_async_ws INTERFACE ../../include)
endif()
