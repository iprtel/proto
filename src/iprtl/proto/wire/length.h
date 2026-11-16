#pragma once

#include "iprtl/proto/wire/types.h"

namespace iprtl::proto::wire {

inline std::size_t length(varint value)  {
    uint64_t v = static_cast<uint64_t>(value.content);
    if (v == 0) {
        return 1;
    }
    auto size = (std::bit_width(v)+6) / 7;
    return size;
}

inline std::size_t length(i32 value)  {
    return sizeof(value.content);
}

inline std::size_t length(i64 value)  {
    return sizeof(value.content);
}

inline std::size_t length(len_data value)  {
    return length(wire::varint{value.content.size()}) + value.content.size();
}

inline std::size_t length(record const& record)  {
    std::size_t size = length(varint{(record.field_number << 3) | static_cast<uint8_t>(record.value.index())});
    size += std::visit([](auto&& arg){
        return length(arg);
    }, record.value);
    return size;
}

inline std::size_t max_length(varint)  {
    return 10;
}

inline std::size_t max_length(i32)  {
    return sizeof(i32::content);
}

inline std::size_t max_length(i64)  {
    return sizeof(i64::content);
}

inline std::size_t max_length(len_data)  {
    return max_length(wire::varint{}) + sizeof(len_data::content);
}


}