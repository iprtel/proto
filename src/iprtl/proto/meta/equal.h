#pragma once

#include "concepts.h"
#include "types.h"

namespace iprtl::proto::meta {

// Field-wise comparison; stops at the first difference.
template<typename ProtoT>
requires is_proto_message_p<ProtoT>
bool equal(typename ProtoT::message_type const& lhs, typename ProtoT::message_type const& rhs) {
    bool same = true;
    ProtoT::for_each_field([&]<typename FieldT>() {
        same = same && FieldT::access(lhs) == FieldT::access(rhs);
    });
    return same;
}

}
