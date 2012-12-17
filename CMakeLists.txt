cmake_minimum_required(VERSION 2.8)
project(qqbot)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

find_package(EV)
find_package(ZLIB)

if(NOT EV_FOUND)
	# USE BUNDLED ONE
	add_subdirectory(libev)
endif()

if(ZLIB_FOUND)
	# USE BUNDLED ONE
	add_subdirectory(zlib)
endif()

add_subdirectory(lwqq)

include_directories(lwqq/src/include/)

find_package(Boost 1.46 REQUIRED COMPONENTS thread filesystem program_options)

add_executable(qqbot main.cpp)

target_link_libraries(qqbot liblwqq)
target_link_libraries(qqbot ${Boost_LIBRARIES})

install(TARGETS qqbot RUNTIME DESTINATION bin)

