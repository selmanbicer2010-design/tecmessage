cmake_minimum_required(VERSION 3.15)
project(tecmessage CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)

add_executable(tecmessage
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/main.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/threadpool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/file.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/network/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network/server.cpp
)

target_include_directories(tecmessage PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tecmessage PRIVATE
    Boost::boost
    Boost::system
)
