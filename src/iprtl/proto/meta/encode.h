#pragma once

#include <cstdint>
#include <variant>
#include <vector>
#include "iprtl/proto/encode.h"
#include "concepts.h"
#include "field_access.h"
#include "length.h"

namespace iprtl::proto::meta {

// Make the single-value overloads visible next to the field/message ones.
using proto::encode;

template<typename ProtoT>
requires (is_proto_field_p<ProtoT> && not is_repeated_field<typename ProtoT::proto_type>)
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::class_type const& value);

template<typename ProtoT>
requires is_proto_field_p<ProtoT> && is_repeated_p<typename ProtoT::proto_type>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::class_type const& value);

template<typename ProtoT>
requires is_proto_field_p<ProtoT> && is_packed_p<typename ProtoT::proto_type>
std::span<uint8_t> encode(std::span<uint8_t> buffer,typename ProtoT::class_type const& value);

template<typename ProtoT>
requires is_proto_message_p<ProtoT>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::cpp_type const& value) {
    auto remaining = buffer;
    std::size_t total_length{};
    auto field_count = std::variant_size<typename ProtoT::message_fields>::value;
    for (auto i=0u; i < field_count; i++){
        auto field = AccessorFactory<typename ProtoT::message_fields>{}.byIndex(i);
        remaining = std::visit([&](auto&& field_access){
            using ProtoFieldT = std::decay_t<decltype(field_access)>;
            return encode<ProtoFieldT>(remaining, value);
        }, *field);
    }
    return remaining;
}

template<typename ProtoT>
requires is_proto_message_p<ProtoT>
std::vector<uint8_t> encode(typename ProtoT::cpp_type const& value) {
    auto size = length<ProtoT>(value);
    std::vector<uint8_t> result;
    result.resize(size);
    auto remaing = encode<ProtoT>({result},value);
    return result;
}

template<typename ProtoT>
requires is_nested_p<ProtoT>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::cpp_type const& value) {
    auto encoded_length = length<typename ProtoT::nested_type>(value);
    auto remaining = encode<int32_p>(buffer, encoded_length);
    remaining = encode<typename ProtoT::nested_type>(remaining, value);
    return remaining;
}

template<typename ProtoT>
requires (is_proto_field_p<ProtoT> && not is_repeated_field<typename ProtoT::proto_type>)
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::class_type const& value) {
    auto field_value = ProtoT::access(value);
    if (field_value == typename ProtoT::member_type{}) {
        return buffer;
    }
    auto field_tag = wire::tag<typename ProtoT::proto_type::wire_type>(ProtoT::number);
    auto remaining = encode(buffer, field_tag);
    remaining = encode<typename ProtoT::proto_type>(remaining, field_value);
    return remaining;
}

template<typename ProtoT>
requires is_proto_field_p<ProtoT> && is_packed_p<typename ProtoT::proto_type>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::class_type const& value) {
    auto field_value = ProtoT::access(value);
    if (field_value == typename ProtoT::member_type{}) {
        return buffer;
    }
    auto field_tag = wire::tag<typename ProtoT::proto_type::wire_type>(ProtoT::number);
    auto remaining = encode(buffer, field_tag);
    remaining = encode<typename ProtoT::proto_type>(remaining, field_value);
    return remaining;
}

template<typename ProtoT>
requires is_proto_field_p<ProtoT> && is_repeated_p<typename ProtoT::proto_type>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename ProtoT::class_type const& value) {
    auto field_tag = wire::tag<typename ProtoT::proto_type::wire_type>(ProtoT::number);
    auto remaining = buffer;
    for (const auto& element : ProtoT::access(value)) {
        remaining = encode(remaining, field_tag);
        remaining = encode<typename ProtoT::proto_type::element_type>(remaining, element);
    }
    return remaining;
}

}
