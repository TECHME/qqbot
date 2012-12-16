cmake_minimum_required(VERSION 2.8)
project(qqbot)

add_executable(qqbot main.c)

install(TARGETS qqbot RUNTIME DESTINATION bin)

include_directories(lwqq/src/include/)