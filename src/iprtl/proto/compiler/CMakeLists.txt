# Tokenizer and parser, shared by protopiler and its tests.
add_library(protopiler-frontend STATIC proto_parser.cpp proto_tokenizer.cpp)
target_link_libraries(protopiler-frontend PUBLIC iprtl::proto)

add_executable(protopiler proto_reader.cpp)
target_link_libraries(protopiler PRIVATE protopiler-frontend argparse::argparse)
set_target_properties(protopiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
