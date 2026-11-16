

prepare_proto(proto/datetime.proto)
prepare_proto(proto/datetime_fixed.proto)


add_executable(datetime-ex src/datetime.cpp ${CMAKE_CURRENT_BINARY_DIR}/include/datetime.proto.h ${CMAKE_CURRENT_BINARY_DIR}/include/datetime_fixed.proto.h)
target_link_libraries(datetime-ex PRIVATE fmt::fmt argparse::argparse)
target_include_directories(datetime-ex PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
