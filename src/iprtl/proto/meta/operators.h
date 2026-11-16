#pragma once

#include <variant>
#include "concepts.h"
#include "field_access.h"

namespace iprtl::proto::meta {

template<typename ProtoT>
requires is_proto_message_p<ProtoT>
bool equal(typename ProtoT::message_type const& lhs, typename ProtoT::message_type const& rhs) {
    auto size = std::variant_size<typename ProtoT::message_fields>::value;
    for (auto i=0u; i < size; i++){
        auto field = AccessorFactory<typename ProtoT::message_fields>{}.byIndex(i);
        bool same = std::visit([&](auto&& field_access){
            using ProtoFieldT = std::decay_t<decltype(field_access)>;
            auto const& lhs_value = ProtoFieldT::access(lhs);
            auto const& rhs_value = ProtoFieldT::access(rhs);
            return lhs_value == rhs_value;
        }, *field);
        if (not same) {return false;}
    }
    return true;
}

}
