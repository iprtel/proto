#pragma once

#include <cstdint>
#include "concepts.h"
#include "wire/length.h"
#include "support.h"
#include "operators.h"


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




template<typename ProtoT>
requires (is_proto_field_p<ProtoT> && not is_repeated_field<typename ProtoT::proto_type>)
std::size_t length(typename ProtoT::class_type const& value);

template<typename ProtoT>
requires is_proto_field_p<ProtoT> && is_repeated_p<typename ProtoT::proto_type>
std::size_t length(typename ProtoT::class_type const& value);

template<typename ProtoT>
requires is_proto_field_p<ProtoT> && is_packed_p<typename ProtoT::proto_type>
std::size_t length(typename ProtoT::class_type const& value);

template<typename ProtoT>
requires is_proto_message_p<ProtoT>
std::size_t length(typename ProtoT::cpp_type const& value) {
    std::size_t total_length{};
    auto field_count = std::variant_size<typename ProtoT::message_fields>::value;
    for (auto i=0u; i < field_count; i++){
        auto field = AccessorFactory<typename ProtoT::message_fields>{}.byIndex(i);
        total_length += std::visit([&](auto&& field_access){
            using ProtoFieldT = std::decay_t<decltype(field_access)>;
            return length<ProtoFieldT>(value);
        }, *field);
    }
    return total_length;
}

template<typename ProtoT>
requires is_nested_p<ProtoT>
std::size_t length(typename ProtoT::cpp_type const& value) {
    auto encoded_length = length<typename ProtoT::nested_type>(value);
    return length<int32_p>(encoded_length) + encoded_length;
}

template<typename ProtoT>
requires (is_proto_field_p<ProtoT> && not is_repeated_field<typename ProtoT::proto_type>)
std::size_t length(typename ProtoT::class_type const& value) {
    auto field_value = ProtoT::access(value);
    if (field_value == typename ProtoT::member_type{}) {
        return 0;
    }
    auto field_tag = wire::tag<typename ProtoT::proto_type::wire_type>(ProtoT::number);
    return length(field_tag) + length<typename ProtoT::proto_type>(field_value);
}

template<typename ProtoT>
requires is_proto_field_p<ProtoT> && is_packed_p<typename ProtoT::proto_type>
std::size_t length(typename ProtoT::class_type const& value) {
    auto field_value = ProtoT::access(value);
    if (field_value == typename ProtoT::member_type{}) {
        return 0;
    }
    auto field_tag = wire::tag<typename ProtoT::proto_type::wire_type>(ProtoT::number);
    return wire::length(field_tag) + length<typename ProtoT::proto_type>(field_value);
}

template<typename ProtoT>
requires is_proto_field_p<ProtoT> && is_repeated_p<typename ProtoT::proto_type>
std::size_t length(typename ProtoT::class_type const& value) {
    auto field_tag = wire::tag<typename ProtoT::proto_type::wire_type>(ProtoT::number);
    auto field_tag_length = wire::length(field_tag);
    std::size_t total_length{};
    for (const auto& element : ProtoT::access(value)) {
        total_length += field_tag_length;
        total_length += length<typename ProtoT::proto_type::element_type>(element);
    }
    return total_length;
}


}