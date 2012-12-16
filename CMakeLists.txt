cmake_minimum_required(VERSION 2.8)
project(qqbot)

add_subdirectory(lwqq)

include_directories(lwqq/src/include/)

find_package(Boost 1.46 REQUIRED COMPONENTS thread filesystem)

add_executable(qqbot main.cpp)

target_link_libraries(qqbot liblwqq)
target_link_libraries(qqbot ${Boost_LIBRARIES})

install(TARGETS qqbot RUNTIME DESTINATION bin)
