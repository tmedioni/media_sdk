cmake_minimum_required(VERSION 3.19)
project(media_sdk)

set(CMAKE_CXX_STANDARD 17)

include_directories(deps)

add_executable(media_sdk main.cpp)
target_link_libraries(media_sdk pthread)