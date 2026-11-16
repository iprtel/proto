#pragma once

#include <type_traits>
#include "types.h"

namespace iprtl::proto {
template<typename T>
concept is_proto_zintegral =
    std::same_as<T, sint32_p> ||
    std::same_as<T, sint64_p>;


template<typename T>
concept is_proto_sintegral =
    std::same_as<T, int32_p> ||
    std::same_as<T, int64_p> ||
    std::same_as<T, sfixed32_p> ||
    std::same_as<T, sfixed64_p>;

template<typename T>
concept is_proto_uintegral =
    std::same_as<T, uint32_p> ||
    std::same_as<T, uint64_p> ||
    std::same_as<T, fixed32_p> ||
    std::same_as<T, fixed64_p>;

template<typename T>
concept is_proto_integral =
    std::same_as<T, bool_p> ||
    is_proto_zintegral<T> ||
    is_proto_sintegral<T> ||
    is_proto_uintegral<T>;

static_assert(is_proto_integral<int32_p>);


template<typename T>
concept is_proto_len_data =
    std::same_as<T, string_p> ||
    std::same_as<T, bytes_p>;



template<typename T>
concept is_encoded_varint =
    std::same_as<T, int32_p> ||
    std::same_as<T, int64_p> ||
    std::same_as<T, sint32_p> ||
    std::same_as<T, sint64_p> ||
    std::same_as<T, uint32_p> ||
    std::same_as<T, uint64_p> ||
    std::same_as<T, bool_p>;

template<typename ProtoT>
concept is_encoded_i32 =
    std::same_as<ProtoT, fixed32_p> ||
    std::same_as<ProtoT, sfixed32_p>;

template<typename ProtoT>
concept is_encoded_i64 =
    std::same_as<ProtoT, fixed64_p> ||
    std::same_as<ProtoT, sfixed64_p>;

template<typename ProtoT>
concept is_encoded_fixed_integer =
    std::same_as<ProtoT, fixed32_p> ||
    std::same_as<ProtoT, sfixed32_p> ||
    std::same_as<ProtoT, fixed64_p> ||
    std::same_as<ProtoT, sfixed64_p>;

template<typename ProtoT>
concept is_encoded_fixed_float =
    std::same_as<ProtoT, float_p> ||
    std::same_as<ProtoT, double_p>;

template<typename ProtoT>
concept is_encoded_fixed =
    is_encoded_fixed_integer<ProtoT> ||
    is_encoded_fixed_float<ProtoT>;


template<typename ProtoT>
concept is_packable =
    is_encoded_varint<ProtoT> ||
    is_encoded_i32<ProtoT> ||
    is_encoded_i64<ProtoT>;

template<typename ProtoT>
concept is_encoded_len_data =
    std::same_as<ProtoT, string_p> ||
    std::same_as<ProtoT, bytes_p>;

template<typename ProtoT, typename EncodedT>
concept is_encoding =
    is_encoded_varint<ProtoT> && std::same_as<EncodedT, wire::varint> ||
    is_encoded_i32<ProtoT> && std::same_as<EncodedT, wire::i32> ||
    is_encoded_i64<ProtoT> && std::same_as<EncodedT, wire::i64> ||
    is_encoded_len_data<ProtoT> && std::same_as<EncodedT, wire::len_data>;


// Primary template: defaults to false
template <typename, template <typename...> class>
struct is_instantiation_of : std::false_type {};

// Specialization: true if T is an instantiation of Template<...>
template <template <typename...> class Template, typename... Args>
struct is_instantiation_of<Template<Args...>, Template> : std::true_type {};

template<typename ProtoT>
concept is_enum_p = is_instantiation_of<ProtoT, enum_p>::value;

template<typename ProtoT>
concept is_packed_p = is_instantiation_of<ProtoT, packed_p>::value;

template<typename ProtoT>
concept is_repeated_p = is_instantiation_of<ProtoT, repeated_p>::value;

template<typename ProtoT>
concept is_repeated_field = is_packed_p<ProtoT> || is_repeated_p<ProtoT>;


template<typename ProtoT>
concept is_packed_repeated = is_packed_p<ProtoT> ||
                             is_repeated_p<ProtoT> && is_packable<typename ProtoT::element_type>;

}
