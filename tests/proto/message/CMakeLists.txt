protopiler_add_library(test-protos
    basic_enums.proto
    basic_floats.proto
    basic_ints.proto
    basic_nested.proto
    basic_repeated.proto
    basic_strings.proto)

# Each name builds <name>-tests from <name>_tests.cpp, with '-' written as '_'
# (basic-enums -> basic-enums-tests from basic_enums_tests.cpp).
foreach(name IN ITEMS
        basic-enums basic-floats basic-ints basic-nested basic-repeated basic-strings
        decode-error message malformed-message)
    string(REPLACE "-" "_" source ${name})
    define_test(${name}-tests ${source}_tests.cpp)
    target_link_libraries(${name}-tests PRIVATE test-protos)
endforeach()
