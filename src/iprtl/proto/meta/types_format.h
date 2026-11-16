#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include "concepts.h"
#include "types.h"

namespace iprtl::proto::meta {
template<typename ProtoT>
requires (is_proto_field_p<ProtoT> && not is_repeated_field<typename ProtoT::proto_type>)
inline auto format_as(typename ProtoT::cpp_type const& value){
   return fmt::format(".{}={}",std::string_view{ProtoT::name.data(),ProtoT::name.size()-1}, value);
}

template<typename ProtoT>
requires is_proto_field_p<ProtoT> && is_repeated_field<typename ProtoT::proto_type>
inline auto format_as(typename ProtoT::cpp_type const& value) {
    return fmt::format(".{}=[{}]",std::string_view{ProtoT::name.data(),ProtoT::name.size()-1}, fmt::join(value,","));
}

template<typename ProtoMessageT>
requires is_proto_message_p<ProtoMessageT>
inline auto format_as(typename ProtoMessageT::cpp_type const& v){
    std::vector<std::string> field_values;
    ProtoMessageT::for_each_field([&]<typename FieldT>() {
        field_values.push_back(format_as<FieldT>(FieldT::access(v)));
    });
    return fmt::format("{{{}}}",fmt::join(field_values,","));
}

}
