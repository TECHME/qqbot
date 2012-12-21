cmake_minimum_required(VERSION 2.8)
project(urdl)

find_package(Boost 1.46 REQUIRED COMPONENTS system)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(include)

add_library(urdl STATIC src/urdl.cpp)

target_link_libraries(urdl ${Boost_LIBRARIES})
target_link_libraries(urdl -lssl)