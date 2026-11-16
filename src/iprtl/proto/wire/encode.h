#pragma once

#include "iprtl/proto/wire/types.h"
#include "iprtl/proto/wire/operations.h"

namespace iprtl::proto::wire {


inline std::span<uint8_t> encode(std::span<uint8_t> buffer, wire::varint value)  {
    auto index = 0u;
    uint64_t v = static_cast<uint64_t>(value);
    do {
        uint8_t byte = static_cast<uint8_t>(v & 0x7F);
        v >>= 7;
        if (v != 0) {
            byte |= 0x80; // More bytes to come
        }
        buffer[index] = byte;
        index++;
    } while (v != 0);

    return buffer.subspan(index);
}

template<typename T>
inline std::span<uint8_t> encode_fixed(std::span<uint8_t> buffer, T value)  {
    auto v = std::bit_cast<std::array<uint8_t, sizeof(T)>>(value);
    std::copy(v.begin(), v.end(), buffer.begin());
    return buffer.subspan(sizeof(T));
}

inline std::span<uint8_t> encode(std::span<uint8_t> buffer, i32 value)  {
    return encode_fixed(buffer, value.content);
}

inline std::span<uint8_t> encode(std::span<uint8_t> buffer, i64 value)  {
    return encode_fixed(buffer, value.content);
}

inline std::span<uint8_t> encode(std::span<uint8_t> buffer, len_data value)  {
    auto& data = value.content;
    buffer = encode(buffer, wire::varint{data.size()});
    std::copy_n(data.data(), data.size(), buffer.data());
    return buffer.subspan(data.size());
}

inline std::span<uint8_t> encode(std::span<uint8_t> buffer, const record& record)  {
    std::visit(
        overloaded{
            [&buffer, field_number=record.field_number](varint value) {
                buffer = encode(buffer, make_tag(field_number, type_id::VARINT));
                buffer = encode(buffer, value);
            },
            [&buffer, field_number=record.field_number](i32 value) {
                buffer = encode(buffer, make_tag(field_number, type_id::I32));
                buffer = encode(buffer, value);
            },
            [&buffer, field_number=record.field_number](i64 value) {
                buffer = encode(buffer, make_tag(field_number, type_id::I64));
                buffer = encode(buffer, value);
            },
            [&buffer, field_number=record.field_number](len_data value) {
                buffer = encode(buffer, make_tag(field_number, type_id::LEN));
                buffer = encode(buffer, value);
            }
        },
        record.value
    );
    return buffer;
}


}