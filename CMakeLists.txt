cmake_minimum_required(VERSION 3.13)
project(IPC_Shell)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

FILE(GLOB sources *.c)
FILE(GLOB headers *.h)

add_executable(dish
        ${sources}
        ${headers})
