cmake_minimum_required(VERSION 2.8)
project(qqbot)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
find_package(Boost 1.46 REQUIRED COMPONENTS date_time thread filesystem program_options)

add_subdirectory(lwqq)
add_subdirectory(libirc)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(lwqq/src/include/)

add_executable(qqbot main.cpp)

target_link_libraries(qqbot liblwqq libirc)
target_link_libraries(qqbot ${Boost_LIBRARIES})

if(WIN32)
	include_directories(win32)
	target_link_libraries(qqbot win32/libpthreadGC2.a)
endif()

install(TARGETS qqbot RUNTIME DESTINATION bin)

