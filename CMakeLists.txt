cmake_minimum_required(VERSION 3.30.5)
project(Anura C)

set(CMAKE_C_STANDARD 99)


add_executable(Anura main.c Array.c
        Sauron.c
        Sauron.h)
target_compile_options(Anura PUBLIC -g)

add_executable(target target.c target.h)
