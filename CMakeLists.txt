cmake_minimum_required(VERSION 3.30.5)
project(Anura C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE Debug)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

add_executable(Anura main.c shared/Array.c
        Sauron.c
        Sauron.h
        Saruman/Saruman.c
        Saruman/Saruman.h
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
        Tolkien.h

        shared/Vector.c
        shared/Helper_File.c
        shared/Helper_String.c
        shared/Helper_Math.c
        shared/Errors.h
        shared/Buffer.c
        Saruman/FrameInfo.c
        Saruman/FrameInfo.h
        Saruman/DWARFParsing.c
        Saruman/DWARFParsing.h
        Saruman/FrameInfoInternal.h
        Saruman/eh_header.h

)

add_executable(MtDoom
        MtDoom/output/default.c
        MtDoom/output/default.h
        MtDoom/output/Generator.c
        MtDoom/output/Generator.h
        MtDoom/output/GeneratorInternal.h
        MtDoom/parser/Parser.c
        MtDoom/parser/Parser.h
        MtDoom/parser/ShuntingYard.c
        MtDoom/parser/ShuntingYard.h
        MtDoom/MtDoom.c
        MtDoom/MtDoom.h
        MtDoom/lexer/Lexer.c
        MtDoom/lexer/Lexer.h

        shared/Vector.c
        shared/Helper_File.c
        shared/Helper_String.c
        shared/Helper_Math.c
        shared/Buffer.c
        shared/Array.c
)

target_link_libraries(MtDoom PRIVATE m)
target_link_options(MtDoom PRIVATE -fsanitize=address)
target_compile_options(MtDoom PUBLIC -fsanitize=address)
target_include_directories(MtDoom PRIVATE MtDoom/output/)

add_executable(x64Diss
    x64/x64.c
    MtDoom/output/default.c
)

target_include_directories(x64Diss PRIVATE MtDoom/output/)

include_directories(.)
include_directories(./shared ./MtDoom)


add_compile_options(-pthread)
find_package(Threads REQUIRED)
target_link_libraries(Anura PRIVATE Threads::Threads)

target_compile_options(Anura PUBLIC -g)
add_executable(test Test.c)
add_executable(Palantir Palantir/Palantir.c Palantir/Palantir.h)


find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

target_include_directories(Palantir PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_libraries(Palantir PRIVATE ${GTK4_LIBRARIES})
target_compile_options(Palantir PRIVATE ${GTK4_CFLAGS_OTHER})
