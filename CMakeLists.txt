cmake_minimum_required(VERSION 3.20)
project(hashmonke LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C,CXX>:/W4>
        $<$<COMPILE_LANGUAGE:C,CXX>:/arch:SSE4.2>
        $<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>
        $<$<COMPILE_LANGUAGE:C,CXX>:/D_CRT_SECURE_NO_WARNINGS>
    )
endif()

if(NOT CMAKE_ASM_MASM_COMPILER OR CMAKE_ASM_MASM_COMPILER STREQUAL "ml")
    find_program(MASM_ML64_PROG NAMES ml64.exe ml64)
    if(NOT MASM_ML64_PROG)
        file(GLOB MASM_ML64_CANDIDATES
            "C:/Program Files/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/bin/Hostx64/x64/ml64.exe"
            "C:/Program Files (x86)/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/bin/Hostx64/x64/ml64.exe"
        )
        if(MASM_ML64_CANDIDATES)
            list(GET MASM_ML64_CANDIDATES 0 MASM_ML64_PROG)
        endif()
    endif()
    if(MASM_ML64_PROG)
        set(CMAKE_ASM_MASM_COMPILER "${MASM_ML64_PROG}" CACHE FILEPATH "MASM compiler" FORCE)
    endif()
endif()

enable_language(ASM_MASM)

set(ASM_SOURCES
    algo/crc32_x86_64.asm
    algo/md5_x86_64.asm
    algo/sha1_x86_64.asm
)

set(CORE_SOURCES
    file.c
    file_sfv.c
    file_gnu.c
    file_bsd.c
    hasher.c
    runner.c
    cli.c
    tui.c
    tui_render.c
    algo/md_internal.c
    algo/md5.c
    algo/sha1.c
    algo/crc32.c
)

add_library(hashmonke_core STATIC
    ${CORE_SOURCES}
    ${ASM_SOURCES}
)

target_include_directories(hashmonke_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/algo
)

if(MSVC)
    target_compile_options(hashmonke_core PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/experimental:c11atomics>)
endif()

add_executable(hashmonke
    main.c
)
target_link_libraries(hashmonke PRIVATE hashmonke_core)
if(MSVC)
    target_compile_options(hashmonke PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/experimental:c11atomics>)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
