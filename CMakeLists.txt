cmake_minimum_required(VERSION 3.20) 
project(firemon VERSION 1.0)  

set(CMAKE_CXX_STANDARD 20)             
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(firemon 
    src/main.cpp
)

target_include_directories(firemon PRIVATE
    include
)

target_compile_options(firemon PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
)
