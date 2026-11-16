#pragma once

#include <cstdint>
#include "concepts.h"
#include "wire/encode.h"
#include "length.h"


namespace iprtl::proto {

template<typename ProtoT>
requires is_encoded_varint<ProtoT>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::cpp_type value) {
    if constexpr (is_proto_zintegral<ProtoT>){
        auto v = static_cast<int64_t>(value);
        return  encode(buffer, wire::varint{static_cast<wire::varint::cpp_type>((v << 1) ^ (v >> 63))});
    }
    return encode(buffer, wire::varint{static_cast<wire::varint::cpp_type>(value)});
}

template<typename ProtoT>
requires is_encoded_fixed_integer<ProtoT>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::cpp_type value) {
    auto v = static_cast<typename ProtoT::wire_type::cpp_type>(value);
    return encode(buffer, typename ProtoT::wire_type{v});
}
template<typename ProtoT>
requires is_encoded_fixed_float<ProtoT>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::cpp_type value) {
    auto v = std::bit_cast<typename ProtoT::wire_type::cpp_type>(value);
    return encode(buffer, typename ProtoT::wire_type{v});
}

template<typename ProtoT>
requires is_enum_p<ProtoT>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::cpp_type value) {
    auto v = static_cast<int32_t>(value);
    return encode<int32_p>(buffer, v);
}

template<typename ProtoT>
requires is_proto_len_data<ProtoT>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::cpp_type const& value) {
    return encode(buffer, typename ProtoT::wire_type{{value.begin(), value.end()}});
}

template<typename ProtoT>
requires is_packed_p<ProtoT>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::cpp_type const& value) {
    auto encoded_length = data_length<ProtoT>(value);
    auto remaining = encode<int32_p>(buffer, encoded_length);
    for (const auto& element : value) {
        remaining = encode<typename ProtoT::element_type>(remaining, element);
    }
    return remaining;
}

}
