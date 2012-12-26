cmake_minimum_required(VERSION 2.8)
project(libwebqq)

find_package(Boost 1.46 REQUIRED COMPONENTS signals system)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(urdl/include/)
include_directories(..)
add_definitions(-DURDL_DISABLE_SSL=1)

add_subdirectory(urdl)

add_library(libwebqq STATIC webqq.cpp webqq_impl.cpp url.cpp logger.cpp md5.c)

target_link_libraries(libwebqq urdl ${Boost_LIBRARIES})

add_executable(test test.cpp)

target_link_libraries(test libwebqq)