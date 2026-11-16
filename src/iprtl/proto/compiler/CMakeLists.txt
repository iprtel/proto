
add_executable(protopiler proto_reader.cpp proto_tokenizer.cpp proto_parser.cpp)
target_link_libraries(protopiler PRIVATE fmt::fmt argparse::argparse)
set_target_properties(protopiler PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)

set_target_properties(protopiler PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)