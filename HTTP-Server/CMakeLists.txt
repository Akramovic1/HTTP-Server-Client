cmake_minimum_required(VERSION 3.21)
project(HTTP_Server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread" )

set(CMAKE_CXX_STANDARD 14)

add_executable(HTTP_Server main.cpp)
