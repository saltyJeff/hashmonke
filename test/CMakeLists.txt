set(TEST_SOURCES
    main.cpp
    test_md5.cpp
    test_sha1.cpp
    test_crc32.cpp
    test_file.cpp
    test_hasher.cpp
    test_runner.cpp
    test_cli.cpp
    test_e2e.cpp
    test_tui.cpp
)

add_executable(test_hashmonke
    ${TEST_SOURCES}
)

target_link_libraries(test_hashmonke PRIVATE hashmonke_core)

target_include_directories(test_hashmonke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME hashmonke_tests COMMAND test_hashmonke)
