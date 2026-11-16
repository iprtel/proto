#pragma once

#include <cstdint>
#include "iprtl/proto/concepts.h"
#include "decode.h"
#include "as_packable.h"

namespace iprtl::proto {

template<typename Type>
requires std::same_as<Type, string_p>
std::string as(wire::len_data const& value) {
    return std::string(reinterpret_cast<const char*>(value.content.data()), value.content.size());
}

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
