cmake_minimum_required(VERSION 2.8)
project(libev)

if(WIN32)

add_definitions(-DEV_USE_IOCP -DEV_SELECT_IS_WINSOCKET)

else()

add_definitions(-DHAVE_FLOOR)
add_definitions(-DHAVE_CLOCK_SYSCALL)
add_definitions(-DHAVE_CLOCK_GETTIME)
add_definitions(-DHAVE_NANOSLEEP)
add_definitions(-DHAVE_EPOLL_CTL -DHAVE_SYS_EPOLL_H)

endif()



add_definitions(-DEV_STANDALONE)
add_library(libev STATIC
	ev.c
	event.c
)