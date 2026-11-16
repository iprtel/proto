#pragma once

#include <cstdint>
#include <variant>
#include "iprtl/proto/length.h"
#include "concepts.h"
#include "field_access.h"

namespace iprtl::proto::meta {

// Make the single-value overloads visible next to the field/message ones.
using proto::length;

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
