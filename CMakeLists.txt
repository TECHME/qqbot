
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
find_package(Boost 1.46 REQUIRED COMPONENTS date_time thread filesystem program_options regex)

add_subdirectory(urdl)
add_subdirectory(libwebqq)
add_subdirectory(lwqq)
add_subdirectory(libirc)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(lwqq/src/include/)
include_directories(urdl/include/)

add_executable(qqbot main.cpp)

target_link_libraries(qqbot libwebqq libirc)
target_link_libraries(qqbot ${Boost_LIBRARIES})

if(WIN32)
	include_directories(win32)
	target_link_libraries(qqbot win32/libpthreadGC2.a)
endif()

install(TARGETS qqbot RUNTIME DESTINATION bin)
