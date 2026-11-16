# define_test(<name> <source>...): a GoogleTest executable registered with CTest.
function(define_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE iprtl::proto GTest::gmock GTest::gtest_main)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_subdirectory(wire)
add_subdirectory(tokenizer)
add_subdirectory(compiler)
add_subdirectory(proto)
