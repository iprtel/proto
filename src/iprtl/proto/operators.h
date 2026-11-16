#pragma once

#include <algorithm>
#include <cstdint>
#include "concepts.h"
#include "wire/length.h"


namespace iprtl::proto {

template<typename ProtoT>
requires is_enum_p<ProtoT>
bool operator==(typename ProtoT::cpp_type lhs, typename ProtoT::cpp_type rhs) {
    return static_cast<int32_t>(lhs) == static_cast<int32_t>(rhs);
}

template<typename ProtoT>
requires is_packed_p<ProtoT> || is_repeated_p<ProtoT>
bool operator==(typename ProtoT::cpp_type lhs, typename ProtoT::cpp_type rhs) {
    return std::ranges::equal(lhs, rhs);
}

}
