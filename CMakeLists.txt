cmake_minimum_required(VERSION 3.30.5)
project(Anura C)

set(CMAKE_C_STANDARD 99)


add_executable(Anura main.c Array.c
        Sauron.c
        Sauron.h
        Saruman.c
        Saruman.h
        Palantir.c
        Palantir.h
        QueueB.c
        QueueB.h
        TargetOS/Linux.c
        TargetOS/Linux.h
        TargetArch/x64.c
        TargetArch/x64.h
        main.h
        IsildursBane.c
        IsildursBane.h
        Target/Linux-x64.c
        Target/Linux-x64.h
        Target.c
        Target.h
        Tolkien.c
        Tolkien.h)

add_compile_options(-pthread)
find_package(Threads REQUIRED)
target_link_libraries(Anura PRIVATE Threads::Threads)

target_compile_options(Anura PUBLIC -g)
add_executable(test Test.c)
