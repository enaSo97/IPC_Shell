cmake_minimum_required(VERSION 3.13)
project(IPC_Shell)

set(CMAKE_CXX_STANDARD 14)

FILE(GLOB sources *.c)
FILE(GLOB headers *.h)

add_executable(dish
        ${sources}
        ${headers})
