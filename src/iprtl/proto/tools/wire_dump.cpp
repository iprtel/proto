// This tool is used to dump the contents of a protobuf message in a human-readable format, showing the field numbers, wire types, and raw values. It can be useful for debugging and understanding the structure of protobuf messages.
// The tool reads a binary protobuf message from standard input and outputs the parsed records to standard output.

#include <iostream>
#include <vector>
#include <span>
#include <fmt/core.h>
#include "iprtl/proto/wire/read.h"
#include "iprtl/proto/wire/types.h"

int main() {
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(std::cin), {});
    auto records = iprtl::proto::wire::read_all_records(buffer);
    for (const auto& record : records) {
        fmt::print("Field Number: {} : {}\n", record.field_number, record.value);
    }
    return 0;
}
