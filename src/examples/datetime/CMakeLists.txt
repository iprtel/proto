protopiler_add_library(datetime-protos proto/datetime.proto proto/datetime_fixed.proto)

add_executable(datetime-ex src/datetime.cpp)
target_link_libraries(datetime-ex PRIVATE datetime-protos argparse::argparse)
