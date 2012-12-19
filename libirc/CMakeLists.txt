cmake_minimum_required(VERSION 2.8)
project(libirc)

find_package(Boost 1.46 REQUIRED COMPONENTS date_time thread)

include_directories(${Boost_INCLUDE_DIRS})

add_library(libirc STATIC irc.cpp)

target_link_libraries(libirc ${Boost_LIBRARIES})

if(WIN32)
	include_directories(${CMAKE_SOURCE_DIR}/win32)
	target_link_libraries(qqbot win32/libpthreadGC2.a)
endif()