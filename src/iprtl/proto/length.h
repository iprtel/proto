#pragma once

#include <cstdint>
#include "concepts.h"
#include "wire/length.h"


namespace iprtl::proto {

template<typename ProtoT>
requires is_encoded_varint<ProtoT>
std::size_t length(typename ProtoT::cpp_type value) {
    if constexpr (is_proto_zintegral<ProtoT>){
        auto v = static_cast<int64_t>(value);
        return  length(wire::varint{static_cast<wire::varint::cpp_type>((v << 1) ^ (v >> 63))});
    }
    return length(wire::varint{static_cast<wire::varint::cpp_type>(value)});
}

template<typename ProtoT>
requires is_encoded_fixed<ProtoT>
std::size_t length(typename ProtoT::cpp_type) {
    return sizeof(typename ProtoT::wire_type::cpp_type);
}

template<typename ProtoT>
requires is_enum_p<ProtoT>
std::size_t length(typename ProtoT::cpp_type value) {
    auto v = static_cast<int32_t>(value);
    return length<int32_p>(v);
}

template<typename ProtoT>
requires is_proto_len_data<ProtoT>
std::size_t length(typename ProtoT::cpp_type const& value) {
    auto size = value.size();
    return length<int32_p>(size) + size;
}

template<typename ProtoT>
requires is_packed_p<ProtoT>
std::size_t data_length(typename ProtoT::cpp_type const& value) {
    std::size_t total_length{};
    for (const auto& element : value) {
        total_length += length<typename ProtoT::element_type>(element);
    }
    return total_length;
}

template<typename ProtoT>
requires is_packed_p<ProtoT>
std::size_t length(typename ProtoT::cpp_type const& value) {
    auto data = data_length<ProtoT>(value);
    return length<int32_p>(data) + data;
}

}
