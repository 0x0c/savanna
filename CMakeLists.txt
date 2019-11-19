cmake_minimum_required(VERSION 3.15.2)
add_definitions("-Wall")
set(CMAKE_CXX_STANDARD 11)

project(savanna VERSION 0.0.1 DESCRIPTION "inter-process notification")
add_library(savanna INTERFACE)

find_package(boost 1.70.0 REQUIRED)

include_directories(${BOOST_ROOT})

target_include_directories(savanna INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

include(GNUInstallDirs)
install(TARGETS savanna
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Add sub-directories
add_subdirectory(example/client)
add_subdirectory(example/websocket)