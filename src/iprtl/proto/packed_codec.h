#pragma once

#include <cstdint>
#include <span>

#include "codec.h"
#include "scalar_codecs.h"
#include "packed_repeated_values.h"

namespace iprtl::proto {

// A packed repeated field's value: a length prefix followed by the elements
// back to back, without tags.
template<typename ProtoT>
requires is_packed_p<ProtoT>
struct codec<ProtoT> {
    using element_type = typename ProtoT::element_type;
    using element_codec = codec<element_type>;
    using value_type = typename ProtoT::cpp_type;
    using wire_type = wire::len_data;

    // Size of the elements, without the length prefix.
    static std::size_t data_length(value_type const& value) {
        std::size_t total_length{};
        for (const auto& element : value) {
            total_length += element_codec::length(element);
        }
        return total_length;
    }
    static std::size_t length(value_type const& value) {
        auto data = data_length(value);
        return codec<int32_p>::length(data) + data;
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type const& value) {
        auto remaining = codec<int32_p>::encode(buffer, data_length(value));
        for (const auto& element : value) {
            remaining = element_codec::encode(remaining, element);
        }
        return remaining;
    }
    static value_type decode(wire::len_data const& value) {
        packed_repeated_values<element_type> values{};
        values.data = value;
        value_type result;
        for (auto element : values) {
            result.push_back(element);
        }
        return result;
    }
};

}
