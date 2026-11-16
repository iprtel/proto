


set(TEST_PROTO_HEADERS "")

macro(prepare proto_file)
    message(STATUS "Generate: ${proto_file}.h from ${proto_file}")
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/${proto_file}.h
        COMMAND ${CMAKE_BINARY_DIR}/bin/protopiler --out-dir "${CMAKE_CURRENT_BINARY_DIR}/include"  ${proto_file}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        DEPENDS protopiler ${proto_file}
    )
    list(APPEND TEST_PROTO_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/include/${proto_file}.h)
#    set(TEST_PROTO_HEADERS "${TEST_PROTO_HEADERS}" PARENT_SCOPE)
endmacro()
prepare(basic_enums.proto)
prepare(basic_floats.proto)
prepare(basic_ints.proto)
prepare(basic_nested.proto)
prepare(basic_strings.proto)
prepare(basic_repeated.proto)
message(STATUS "TEST_PROTO_HEADERS: ${TEST_PROTO_HEADERS}")

add_custom_target(test_proto_headers ALL DEPENDS ${TEST_PROTO_HEADERS}
    COMMENT "Generating proto headers"
)

define_test(basic-enums-tests basic_enums_tests.cpp ${CMAKE_CURRENT_BINARY_DIR}/include/basic_enums.proto.h)
target_include_directories(basic-enums-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

define_test(basic-floats-tests basic_floats_tests.cpp ${CMAKE_CURRENT_BINARY_DIR}/include/basic_floats.proto.h)
target_include_directories(basic-floats-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

define_test(basic-ints-tests basic_ints_tests.cpp ${CMAKE_CURRENT_BINARY_DIR}/include/basic_ints.proto.h)
target_include_directories(basic-ints-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

define_test(basic-nested-tests basic_nested_tests.cpp ${CMAKE_CURRENT_BINARY_DIR}/include/basic_nested.proto.h)
target_include_directories(basic-nested-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

define_test(basic-repeated-tests basic_repeated_tests.cpp ${CMAKE_CURRENT_BINARY_DIR}/include/basic_repeated.proto.h)
target_include_directories(basic-repeated-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

define_test(basic-strings-tests basic_strings_tests.cpp ${CMAKE_CURRENT_BINARY_DIR}/include/basic_strings.proto.h)
target_include_directories(basic-strings-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

define_test(decode-error-tests decode_error_tests.cpp ${CMAKE_CURRENT_BINARY_DIR}/include/basic_floats.proto.h)
target_include_directories(decode-error-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

define_test(message-tests message_tests.cpp ${TEST_PROTO_HEADERS})
target_include_directories(message-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

define_test(malformed-message-tests malformed_message_tests.cpp ${TEST_PROTO_HEADERS})
target_include_directories(malformed-message-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
