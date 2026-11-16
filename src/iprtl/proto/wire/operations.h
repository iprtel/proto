#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include "types.h"
#include "iprtl/enums.h"
#include "iprtl/template_string.h"
#include "fmt/core.h"
#include "fmt/ranges.h"

namespace iprtl::proto::wire {

template<typename WireType, typename CppType>
record make_record(uint32_t number, CppType const& value) {
    record record{};
    record.field_number = number;
    record.value.emplace<WireType>(static_cast<typename WireType::cpp_type>(value));
    return record;
}
template<typename WireType>
record make_record(uint32_t number, std::string const& value) {
    record record{};
    record.field_number = number;
    record.value.emplace<WireType>(std::vector<uint8_t>(value.begin(), value.end()));
    return record;
}

template<typename WireType>
record make_record(uint32_t number, std::vector<uint8_t> const& value) {
    record record{};
    record.field_number = number;
    record.value.emplace<WireType>(std::vector<uint8_t>(value.begin(), value.end()));
    return record;
}

inline bool operator==(const record&  lhs, const record& rhs) {
    return (lhs.field_number == rhs.field_number) && (lhs.value == rhs.value);
}

inline constexpr varint make_tag(uint32_t field_number,  type_id wire_type) {
    return varint{(field_number << 3) | static_cast<uint8_t>(wire_type)};
}

//template<typename T>
//inline constexpr len_data make_packed(std::vector<T> list){
//    len_data result{};
//    for(auto value: list){
//        auto len =
//    }
//}


template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };



}