cmake_minimum_required(VERSION 3.15.0)
add_definitions("-Wall")
set(CMAKE_CXX_STANDARD 11)

project(savanna VERSION 0.0.5 DESCRIPTION "boost-beast wrapper")
add_library(savanna INTERFACE)

if (APPLE)
find_package(boost 1.70.0 REQUIRED)
else()
find_package(Boost 1.70.0 REQUIRED)
endif()

include_directories(${BOOST_ROOT})

target_include_directories(savanna INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

include(GNUInstallDirs)
install(TARGETS savanna
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Add sub-directories
add_subdirectory(example/client)
add_subdirectory(example/websocket)
