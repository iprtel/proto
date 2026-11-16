#pragma once

#include <cstdint>
#include "iprtl/proto/concepts.h"
#include "decode.h"
#include "iprtl/proto/decode.h"
#include "iprtl/proto/proto_exception.h"

namespace iprtl::proto {

namespace meta {
template<typename ProtoMessageT>
requires is_proto_message_p<ProtoMessageT>
std::span<const uint8_t> decode(std::span<const uint8_t> buffer,typename ProtoMessageT::cpp_type & message);
}

template<typename Type>
requires is_nested_p<Type>
Type::cpp_type as(wire::len_data value) {
    typename Type::nested_type::cpp_type v{};
    auto remaining = meta::decode<typename Type::nested_type>(value.content, v);
    if (not remaining.empty()) throw proto_exception("nested message length mismatch");
    return v;
}

}
