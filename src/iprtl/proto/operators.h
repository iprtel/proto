#pragma once

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

template<typename ProtoT>
requires is_proto_message_p<ProtoT>
bool operator==(typename ProtoT::message_type const& lhs, typename ProtoT::message_type const& rhs) {
    auto size = std::variant_size<typename ProtoT::message_fields>::value;
    for (auto i=0u; i < size; i++){
        auto field = AccessorFactory<typename ProtoT::message_fields>{}.byIndex(i);
        bool equal = std::visit([&](auto&& field_access){
            using ProtoFieldT = std::decay_t<decltype(field_access)>;
            auto const& lhs_value = ProtoFieldT::access(lhs);
            auto const& rhs_value = ProtoFieldT::access(rhs);
            return lhs_value == rhs_value;
        }, *field);
        if (not equal) {return false;}
    }
    return true;
};

template<typename ProtoT>
requires is_proto_field_p<ProtoT>
bool operator==(typename ProtoT::message_type const& lhs, typename ProtoT::message_type const& rhs) {
    return false;
}


}