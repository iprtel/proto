add_subdirectory(message)

define_test(length-tests length_tests.cpp)
define_test(encode-tests encode_tests.cpp)
define_test(packed-repeated-values-tests packed_repeated_values_tests.cpp)
