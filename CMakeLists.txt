cmake_minimum_required(VERSION 4.0)
project(nemo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE sources src/*.c include/*.h)

add_executable(nemo ${sources})

target_compile_options(nemo PUBLIC -Werror)
target_include_directories(nemo PUBLIC ./include)
