#pragma once

#include <tuple>

#include "wire_exception.h"

#include "iprtl/proto/wire/types.h"

namespace iprtl::proto::wire {

inline std::tuple<type_id, uint64_t> parse_tag(varint tag) {
    auto tag_value = tag.content;
    auto wire_type = type_id{static_cast<uint8_t>(tag_value & 0x07)};
    uint64_t field_number = tag_value >> 3;
    return {wire_type, field_number};
}

inline std::tuple<std::span<const uint8_t>, wire::varint> read_varint(std::span<const uint8_t> buffer)  {
    // A 64-bit value needs at most 10 bytes (7 payload bits per byte).
    constexpr auto max_varint_bytes = 10u;
    auto index = 0u;
    uint64_t value = 0;
    for(uint64_t data : buffer){
        if (index == max_varint_bytes) {
            throw wire_exception("varint longer than 10 bytes");
        }
        auto part = data & 0x7Fu;
        value |= part << (7 * index);
        index++;
        auto last_byte = (data & 0x80) == 0;
        if (last_byte) {
            return {buffer.subspan(index), wire::varint{value}};
        }
    }
    throw wire_exception("truncated varint");
}

template<typename T>
std::tuple<std::span<const uint8_t>, T> read_fixed(std::span<const uint8_t> buffer)  {
    using uint_type = decltype(T::content);
    if (buffer.size() < sizeof(uint_type)) {
        throw wire_exception(fmt::format("expected {} bytes, got {}", sizeof(uint_type), buffer.size()));
    }
    uint_type value{};
    auto appender = [&value,index=0](uint_type byte) mutable {
        value |= byte << (8 * index);
        index++;
    };
    std::for_each_n(buffer.begin(), sizeof(uint_type),  appender);
    return {buffer.subspan(sizeof(uint_type)), T{.content=value}};
}

inline std::tuple<std::span<const uint8_t>, i32> read_i32(std::span<const uint8_t> buffer)  {
    return read_fixed<i32>(buffer);
}

inline std::tuple<std::span<const uint8_t>, i64> read_i64(std::span<const uint8_t> buffer)  {
    return read_fixed<i64>(buffer);
}

inline std::tuple<std::span<const uint8_t>, len_data> read_len_data(std::span<const uint8_t> buffer)  {
    auto  [remaining,length_value] = read_varint(buffer);
    uint64_t length = static_cast<uint64_t>(length_value);
    if (length > remaining.size()) throw wire_exception(fmt::format("expected length: {}", length));
    len_data result{};
    auto & data = result.content;
    data.resize(length);
    std::copy_n(remaining.data(), length, data.data());
    return {remaining.subspan(length), result};
}

inline std::tuple<std::span<const uint8_t>, record> read_record(std::span<const uint8_t> buffer)  {
    auto [remaining, tag] = read_varint(buffer);
    auto [type, field_number] = parse_tag(tag);
    if (field_number == 0 || field_number > 0x1FFFFFFF) throw wire_exception("field number must be non-zero and <= 2^29-1");
    switch(type) {
        case wire::type_id::VARINT: {
            varint value{};
            std::tie(remaining, value) = read_varint(remaining);
            return {remaining, {field_number, value}};
        }
        case wire::type_id::I32: {
            i32 value{};
            std::tie(remaining, value) = read_i32(remaining);
            return {remaining, {field_number, value}};
        }
        case wire::type_id::I64: {
            i64 value{};
            std::tie(remaining, value) = read_i64(remaining);
            return {remaining,  {field_number, value}};
        }
        case wire::type_id::LEN: {
            len_data data{};
            std::tie(remaining, data) = read_len_data(remaining);
            return {remaining,  {field_number, data}};
        }
        default:
            throw wire_exception(fmt::format("Unsupported wire type: {}", static_cast<uint8_t>(type)));
    }
}

std::vector<record> read_all_records(std::span<const uint8_t> buffer) {
    std::vector<record> records;
    while (!buffer.empty()) {
        auto [remaining, record] = read_record(buffer);
        records.push_back(record);
        buffer = remaining;
    }
    return records;
}

} // namespace iprtl::proto::wire
