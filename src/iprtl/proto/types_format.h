#pragma once

#include "types.h"
#include "concepts.h"

namespace iprtl::proto {

inline auto format_as(int32_p){
    return "int32";
}
inline auto format_as(int64_p){
    return "int64";
}
inline auto format_as(uint32_p){
    return "uint32";
}
inline auto format_as(uint64_p){
    return "uint64";
}
inline auto format_as(sint32_p){
    return "sint32";
}
inline auto format_as(sint64_p){
    return "sint64";
}
inline auto format_as(fixed32_p){
    return "fixed32";
}
inline auto format_as(fixed64_p){
    return "fixed64";
}
inline auto format_as(sfixed32_p){
    return "sfixed32";
}
inline auto format_as(sfixed64_p){
    return "sfixed64";
}
inline auto format_as(bool_p){
    return "bool";
}
inline auto format_as(double_p){
    return "double";
}
inline auto format_as(float_p){
    return "float";
}
inline auto format_as(string_p){
    return "string";
}
inline auto format_as(bytes_p){
    return "bytes";
}
template<typename T>
inline auto format_as(enum_p<T>){
    return "enum";
}
template<typename ProtoT>
requires is_repeated_p<ProtoT> || is_packed_p<ProtoT>
inline auto format_as(ProtoT){
    return std::string("repeated ").append(format_as<ProtoT::element_type>({}));
}
}