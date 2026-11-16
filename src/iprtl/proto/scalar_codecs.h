#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <span>
#include <string>

#include "codec.h"
#include "concepts.h"
#include "types.h"
#include "wire/encode.h"
#include "wire/length.h"

namespace iprtl::proto {

// int32, int64, uint32, uint64, sint32, sint64 (ZigZag), bool
template<typename ProtoT>
requires is_encoded_varint<ProtoT>
struct codec<ProtoT> {
    using value_type = typename ProtoT::cpp_type;
    using wire_type = wire::varint;

    static wire::varint to_wire(value_type value) {
        if constexpr (is_proto_zintegral<ProtoT>) {
            auto v = static_cast<int64_t>(value);
            return wire::varint{static_cast<wire::varint::cpp_type>((v << 1) ^ (v >> 63))};
        } else {
            return wire::varint{static_cast<wire::varint::cpp_type>(value)};
        }
    }
    static std::size_t length(value_type value) {
        return wire::length(to_wire(value));
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type value) {
        return wire::encode(buffer, to_wire(value));
    }
    static value_type decode(wire::varint value) {
        auto v = static_cast<uint64_t>(value);
        if constexpr (is_proto_zintegral<ProtoT>) {
            return static_cast<value_type>((v >> 1) ^ (-(v & 1)));
        } else if constexpr (std::same_as<ProtoT, bool_p>) {
            return v != 0;
        } else {
            return static_cast<value_type>(v);
        }
    }
};

// fixed32, sfixed32, fixed64, sfixed64
template<typename ProtoT>
requires is_encoded_fixed_integer<ProtoT>
struct codec<ProtoT> {
    using value_type = typename ProtoT::cpp_type;
    using wire_type = typename ProtoT::wire_type;

    static std::size_t length(value_type) {
        return sizeof(typename wire_type::cpp_type);
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type value) {
        return wire::encode(buffer, wire_type{static_cast<typename wire_type::cpp_type>(value)});
    }
    static value_type decode(wire_type value) {
        return static_cast<value_type>(value);
    }
};

// float, double
template<typename ProtoT>
requires is_encoded_fixed_float<ProtoT>
struct codec<ProtoT> {
    using value_type = typename ProtoT::cpp_type;
    using wire_type = typename ProtoT::wire_type;

    static std::size_t length(value_type) {
        return sizeof(typename wire_type::cpp_type);
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type value) {
        return wire::encode(buffer, wire_type{std::bit_cast<typename wire_type::cpp_type>(value)});
    }
    static value_type decode(wire_type value) {
        return std::bit_cast<value_type>(value.content);
    }
};

// Enums are written like int32.
template<typename ProtoT>
requires is_enum_p<ProtoT>
struct codec<ProtoT> {
    using value_type = typename ProtoT::cpp_type;
    using wire_type = wire::varint;

    static std::size_t length(value_type value) {
        return codec<int32_p>::length(static_cast<int32_t>(value));
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type value) {
        return codec<int32_p>::encode(buffer, static_cast<int32_t>(value));
    }
    static value_type decode(wire::varint value) {
        return static_cast<value_type>(codec<uint32_p>::decode(value));
    }
};

// string, bytes: a length prefix followed by the raw bytes.
template<typename ProtoT>
requires is_proto_len_data<ProtoT>
struct codec<ProtoT> {
    using value_type = typename ProtoT::cpp_type;
    using wire_type = wire::len_data;

    static std::size_t length(value_type const& value) {
        auto size = value.size();
        return codec<int32_p>::length(size) + size;
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type const& value) {
        return wire::encode(buffer, wire::len_data{{value.begin(), value.end()}});
    }
    static value_type decode(wire::len_data const& value) {
        if constexpr (std::same_as<ProtoT, string_p>) {
            return std::string(reinterpret_cast<const char*>(value.content.data()), value.content.size());
        } else {
            return value.content;
        }
    }
};

}
