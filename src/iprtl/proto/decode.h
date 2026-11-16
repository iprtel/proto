#pragma once

#include <bit>
#include <cstdint>
#include <string>
#include "concepts.h"

namespace iprtl::proto {

inline constexpr uint64_t operator "" _z( unsigned long long int value){
    auto v = static_cast<int64_t>(value);
    return (v << 1) ^ (v >> 63);
}

template<typename Type>
requires is_proto_sintegral<Type> || is_proto_uintegral<Type>
typename Type::cpp_type as(typename Type::wire_type value) {
    return static_cast<Type::cpp_type>(value);
}

template<typename Type>
requires is_proto_zintegral<Type>
Type::cpp_type as(typename Type::wire_type value) {
    auto v = static_cast<uint64_t>(value);
    return (v >> 1) ^ (-(v & 1));
}

template<typename Type>
requires std::same_as<Type, bool_p>
bool as(auto value) {
    auto v = static_cast<uint64_t>(value);
    return v != 0;
}

template<typename Type>
requires std::same_as<Type, float_p> or std::same_as<Type, double_p>
Type::cpp_type as(typename Type::wire_type value) {
    return std::bit_cast<typename Type::cpp_type>(value.content);
}

template<typename Type>
requires std::is_enum<typename Type::cpp_type>::value
Type::cpp_type as(wire::varint value) {
    auto v = as<uint32_p>(value);
    return typename Type::cpp_type(v);
}

template<typename Type>
requires std::same_as<Type, string_p>
std::string as(wire::len_data const& value) {
    return std::string(reinterpret_cast<const char*>(value.content.data()), value.content.size());
}

}
