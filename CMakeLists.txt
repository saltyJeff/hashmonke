cmake_minimum_required(VERSION 3.20)
project(hashmonke LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(
        /W4
        /O2
        /arch:SSE4.2
        /utf-8
        /D_CRT_SECURE_NO_WARNINGS
    )
endif()

# Find clang for assembling GNU .S assembly files with x86-64-v2 target
find_program(CLANG_ASM_COMPILER NAMES clang clang.exe REQUIRED
    DOC "Path to clang compiler used for assembling .S files")

set(ASM_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/algo/crc32_x86_64.S"
    "${CMAKE_CURRENT_SOURCE_DIR}/algo/md5_x86_64.S"
    "${CMAKE_CURRENT_SOURCE_DIR}/algo/sha1_x86_64.S"
)

set(ASM_OBJECTS "")
foreach(ASM_SRC IN LISTS ASM_SOURCES)
    get_filename_component(ASM_NAME "${ASM_SRC}" NAME_WLE)
    set(ASM_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${ASM_NAME}.obj")
    add_custom_command(
        OUTPUT "${ASM_OBJ}"
        COMMAND "${CLANG_ASM_COMPILER}" --target=x86_64-pc-windows-msvc -march=x86-64-v2 -c "${ASM_SRC}" -o "${ASM_OBJ}"
        DEPENDS "${ASM_SRC}"
        COMMENT "Assembling ${ASM_NAME}.S with clang (target: x86-64-v2)"
    )
    list(APPEND ASM_OBJECTS "${ASM_OBJ}")
endforeach()

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
    ${ASM_OBJECTS}
)

target_include_directories(hashmonke_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/algo
)

if(MSVC)
    target_compile_options(hashmonke_core PRIVATE /experimental:c11atomics)
endif()

add_executable(hashmonke
    main.c
)
target_link_libraries(hashmonke PRIVATE hashmonke_core)
if(MSVC)
    target_compile_options(hashmonke PRIVATE /experimental:c11atomics)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
