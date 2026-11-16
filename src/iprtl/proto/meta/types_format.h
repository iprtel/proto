#pragma once

#include "types.h"
#include "iprtl/proto/support.h"
#include "iprtl/proto/concepts.h"

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
    auto size = std::variant_size<typename ProtoMessageT::message_fields>::value;
    for (auto i=0u; i < size; i++){
        auto field = AccessorFactory<typename ProtoMessageT::message_fields>{}.byIndex(i);
        std::visit([&](auto&& field_access){
            using AccessorT = std::decay_t<decltype(field_access)>;
            field_values.push_back( format_as<AccessorT>(AccessorT::access(v)));
        }, *field);
    }
    return fmt::format("{{{}}}",fmt::join(field_values,","));
}

}