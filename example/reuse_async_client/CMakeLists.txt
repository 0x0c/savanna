cmake_minimum_required(VERSION 3.15.0)

project(simple_reuse_async_client)
set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

find_package(OpenSSL REQUIRED)
if (OpenSSL_FOUND)
    add_executable(simple_reuse_async_client main.cpp)
    target_link_libraries(
        simple_reuse_async_client
        savanna
        ${OPENSSL_CRYPTO_LIBRARY}
        ${OPENSSL_SSL_LIBRARY}
        Threads::Threads
    )
    target_include_directories(simple_reuse_async_client INTERFACE ../../include)
endif()
