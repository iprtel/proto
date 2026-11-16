#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace iprtl::proto {

// codec<ProtoT> says how to measure, write and read one proto type.
//
// proto/ specializes it for single values (scalar_codecs.h, packed_codec.h).
// meta/ adds fields, messages and nested messages (meta/codec.h) without
// proto/ having to know about them. A type without a specialization is not
// supported. Every specialization provides:
//
//   using value_type = ...;                  // what length/encode take
//   static std::size_t length(value_type const&);
//   static std::span<uint8_t> encode(std::span<uint8_t>, value_type const&);
//   static ... decode(...);                  // signature depends on the kind
//
// Specializations are looked up when a codec is used, not when it is
// declared, so the order in which headers define them does not matter.
template<typename ProtoT>
struct codec;

template<typename ProtoT>
std::size_t length(typename codec<ProtoT>::value_type const& value) {
    return codec<ProtoT>::length(value);
}

template<typename ProtoT>
std::span<uint8_t> encode(std::span<uint8_t> buffer, typename codec<ProtoT>::value_type const& value) {
    return codec<ProtoT>::encode(buffer, value);
}

// Values:   decode<P>(wire_value)             -> value
// Messages: decode<M>(buffer, message)        -> remaining buffer
// Fields:   decode<F>(object, wire_value)     -> stores into object
template<typename ProtoT, typename... Args>
decltype(auto) decode(Args&&... args) {
    return codec<ProtoT>::decode(std::forward<Args>(args)...);
}

}
