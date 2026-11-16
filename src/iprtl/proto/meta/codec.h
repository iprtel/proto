#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <variant>

#include "iprtl/proto/codec.h"
#include "iprtl/proto/packed_codec.h"
#include "iprtl/proto/proto_exception.h"
#include "iprtl/proto/wire/encode.h"
#include "iprtl/proto/wire/length.h"
#include "iprtl/proto/wire/read.h"
#include "iprtl/proto/wire/wire_exception.h"
#include "concepts.h"
#include "types.h"

// codec specializations for messages, nested messages and fields.
// They live in iprtl::proto because that is where codec is declared.
namespace iprtl::proto {

// A message: its fields one after another, without a length prefix.
template<typename ClassT, meta::FixedString Name, typename... Fields>
struct codec<meta::ProtoMessage<ClassT, Name, Fields...>> {
    using value_type = ClassT;

    static std::size_t length(value_type const& message) {
        return (std::size_t{0} + ... + codec<Fields>::length(message));
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type const& message) {
        ((buffer = codec<Fields>::encode(buffer, message)), ...);
        return buffer;
    }
    // Reads records until the buffer is empty. Unknown field numbers are skipped.
    static std::span<const uint8_t> decode(std::span<const uint8_t> buffer, value_type& message) {
        while (not buffer.empty()) {
            auto [remaining, record] = wire::read_record(buffer);
            buffer = remaining;
            // Hand the value to the first field with a matching number.
            ((record.field_number == Fields::number
                && (codec<Fields>::decode(message, record.value), true)) || ...);
        }
        return buffer;
    }
};

// A message used as a field value: a length prefix followed by the message.
template<typename MessageT>
struct codec<meta::nested_p<MessageT>> {
    using value_type = typename MessageT::cpp_type;
    using wire_type = wire::len_data;

    static std::size_t length(value_type const& value) {
        auto size = codec<MessageT>::length(value);
        return codec<int32_p>::length(size) + size;
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type const& value) {
        auto remaining = codec<int32_p>::encode(buffer, codec<MessageT>::length(value));
        return codec<MessageT>::encode(remaining, value);
    }
    static value_type decode(wire::len_data const& value) {
        value_type message{};
        auto remaining = codec<MessageT>::decode(value.content, message);
        if (not remaining.empty()) throw proto_exception("nested message length mismatch");
        return message;
    }
};

// A singular field: tag + value. Fields holding the default value are not written.
template<typename FieldT>
requires meta::is_proto_field_p<FieldT> && (not is_repeated_field<typename FieldT::proto_type>)
struct codec<FieldT> {
    using value_type = typename FieldT::class_type;
    using value_codec = codec<typename FieldT::proto_type>;

    static wire::varint tag() {
        return wire::tag<typename FieldT::wire_type>(FieldT::number);
    }
    static std::size_t length(value_type const& object) {
        auto const& value = FieldT::access(object);
        if (value == typename FieldT::member_type{}) {
            return 0;
        }
        return wire::length(tag()) + value_codec::length(value);
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type const& object) {
        auto const& value = FieldT::access(object);
        if (value == typename FieldT::member_type{}) {
            return buffer;
        }
        auto remaining = wire::encode(buffer, tag());
        return value_codec::encode(remaining, value);
    }
    static void decode(value_type& object, wire::wire_value const& value) {
        if (not std::holds_alternative<typename FieldT::wire_type>(value)) throw wire::wire_exception(FieldT::name.data());
        FieldT::access(object) = typename FieldT::member_type{value_codec::decode(std::get<typename FieldT::wire_type>(value))};
    }
};

// A repeated, unpacked field: tag + value for every element.
template<typename FieldT>
requires meta::is_proto_field_p<FieldT> && is_repeated_p<typename FieldT::proto_type>
struct codec<FieldT> {
    using value_type = typename FieldT::class_type;
    using element_codec = codec<typename FieldT::proto_type::element_type>;

    static wire::varint tag() {
        return wire::tag<typename FieldT::wire_type>(FieldT::number);
    }
    static std::size_t length(value_type const& object) {
        auto tag_length = wire::length(tag());
        std::size_t total_length{};
        for (const auto& element : FieldT::access(object)) {
            total_length += tag_length + element_codec::length(element);
        }
        return total_length;
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type const& object) {
        auto remaining = buffer;
        for (const auto& element : FieldT::access(object)) {
            remaining = wire::encode(remaining, tag());
            remaining = element_codec::encode(remaining, element);
        }
        return remaining;
    }
    // Each record adds one element. Records with a different wire type throw wire_exception.
    static void decode(value_type& object, wire::wire_value const& value) {
        using element_type = typename FieldT::proto_type::element_type;
        auto& elements = FieldT::access(object);
        if (auto const* single = std::get_if<typename FieldT::wire_type>(&value)) {
            elements.push_back(element_codec::decode(*single));
            return;
        }
        if constexpr (is_packable<element_type>) {
            if (auto const* packed = std::get_if<wire::len_data>(&value)) {
                for (auto element : codec<packed_p<element_type>>::decode(*packed)) {
                    elements.push_back(element);
                }
                return;
            }
        }
        throw wire::wire_exception(FieldT::name.data());
    }
};

// A packed repeated field: one tag + the packed value. Empty lists are not written.
template<typename FieldT>
requires meta::is_proto_field_p<FieldT> && is_packed_p<typename FieldT::proto_type>
struct codec<FieldT> {
    using value_type = typename FieldT::class_type;
    using value_codec = codec<typename FieldT::proto_type>;

    static wire::varint tag() {
        return wire::tag<typename FieldT::wire_type>(FieldT::number);
    }
    static std::size_t length(value_type const& object) {
        auto const& value = FieldT::access(object);
        if (value == typename FieldT::member_type{}) {
            return 0;
        }
        return wire::length(tag()) + value_codec::length(value);
    }
    static std::span<uint8_t> encode(std::span<uint8_t> buffer, value_type const& object) {
        auto const& value = FieldT::access(object);
        if (value == typename FieldT::member_type{}) {
            return buffer;
        }
        auto remaining = wire::encode(buffer, tag());
        return value_codec::encode(remaining, value);
    }
    // Each record appends its elements, so a list split over several records is joined.
    // Records that are not len_data throw wire_exception.
    static void decode(value_type& object, wire::wire_value const& value) {
        using element_type = typename FieldT::proto_type::element_type;
        auto& elements = FieldT::access(object);
        if (auto const* packed = std::get_if<wire::len_data>(&value)) {
            for (auto element : value_codec::decode(*packed)) {
                elements.push_back(element);
            }
        } else if (auto const* single = std::get_if<typename element_type::wire_type>(&value)) {
            elements.push_back(codec<element_type>::decode(*single));
        } else {
            throw wire::wire_exception(FieldT::name.data());
        }
    }
};

}
