
set(TEST_LIBS
        gtest
        gmock
        gtest_main
        pthread
        fmt::fmt
)

set (SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/../src")

set (TEST_INCLUDE_DIRS
        "${SRC_DIR}"
        "${PROTO_BINARY_DIR}"
)



macro(define_test test_target test_source)
    set(extra_args ${ARGN})
    add_executable(${test_target} ${test_source} ${extra_args})
    target_link_libraries(${test_target} PRIVATE ${TEST_LIBS})
    target_include_directories(${test_target} PRIVATE ${TEST_INCLUDE_DIRS})
    add_test(${test_target} ${test_target})
endmacro()

add_subdirectory(wire)

add_subdirectory(tokenizer)
add_subdirectory(compiler)
add_subdirectory(proto)
