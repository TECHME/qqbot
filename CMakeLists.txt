cmake_minimum_required(VERSION 2.8)
project(qqbot)

add_subdirectory(lwqq)

include_directories(lwqq/src/include/)

add_executable(qqbot main.cpp)

target_link_libraries(qqbot liblwqq)

install(TARGETS qqbot RUNTIME DESTINATION bin)
