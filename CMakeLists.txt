cmake_minimum_required(VERSION 3.15.0)
add_definitions("-Wall")
set(CMAKE_CXX_STANDARD 11)

project(savanna VERSION 0.0.14 DESCRIPTION "boost-beast wrapper")

find_package(Boost 1.70.0 REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
if(Boost_FOUND)
    add_library(savanna INTERFACE)
    include_directories(${Boost_ROOT} ${Boost_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})

    target_include_directories(savanna INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    include(GNUInstallDirs)
    install(
        TARGETS savanna
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Add sub-directories
    add_subdirectory(example/client)
    add_subdirectory(example/async_client)
    add_subdirectory(example/reuse_async_client)
    add_subdirectory(example/websocket)
    add_subdirectory(example/async_websocket)
endif()
