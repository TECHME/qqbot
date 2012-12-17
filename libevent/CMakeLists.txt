cmake_minimum_required(VERSION 2.8)
project(libevent)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

if(WIN32)

include_directories("WIN32-Code")

set(SYS_SRC
	win32select.c evthread_win32.c buffer_iocp.c
	event_iocp.c bufferevent_async.c
)

else()
set(SYS_SRC
	epoll.c
)
endif()

set(CORE_SRC
	event.c evthread.c buffer.c
	bufferevent.c bufferevent_sock.c bufferevent_filter.c
	bufferevent_pair.c listener.c bufferevent_ratelim.c
	evmap.c	log.c evutil.c evutil_rand.c strlcpy.c ${SYS_SRC}
)

set(libevent_SOURCES ${CORE_SRC})

add_library(libevent STATIC ${libevent_SOURCES})