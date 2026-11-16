#pragma once

// Entry point for generated code: whole-message encode/decode/length/compare/format.

#include <optional>
#include <span>
#include <vector>

#include "meta/types.h"
#include "meta/types_format.h"
#include "meta/decode.h"
#include "meta/length.h"
#include "meta/encode.h"
#include "meta/operators.h"


namespace iprtl::proto {

template<typename MetaMessageT>
std::span<uint8_t> encode_message(typename MetaMessageT::message_type message, std::span<uint8_t> buffer) {
    auto remaining = meta::encode<MetaMessageT>(buffer, message);
    return std::span<uint8_t>(buffer.data(), buffer.size() - remaining.size());
};
template<typename MetaMessageT>
std::vector<uint8_t> encode_message(typename MetaMessageT::message_type message) {
    auto size = meta::length<MetaMessageT>(message);
    std::vector<uint8_t> result(size);
    auto remaining = meta::encode<MetaMessageT>(std::span(result), message);
    return result;
};
template<typename MetaMessageT>
std::span<const uint8_t> decode_message(typename MetaMessageT::message_type& message, std::span<const uint8_t> buffer) {
    auto remaining = meta::decode<MetaMessageT>(buffer, message);
    return remaining;
};
template<typename MetaMessageT>
std::optional<typename MetaMessageT::message_type> decode_message(std::span<const uint8_t> buffer) {
    typename MetaMessageT::message_type message{};
    auto remaining = meta::decode<MetaMessageT>(buffer, message);
    if (remaining.size() < buffer.size()){
        return message;
    }
    return {};
};
template<typename MetaMessageT>
std::size_t encoded_length(typename MetaMessageT::message_type const& message) {
    return meta::length<MetaMessageT>(message);
}
template<typename MetaMessageT>
bool messages_equal(typename MetaMessageT::message_type const& lhs, typename MetaMessageT::message_type const& rhs) {
    return meta::equal<MetaMessageT>(lhs, rhs);
}
template<typename MetaMessageT>
auto format_message(typename MetaMessageT::message_type const& message) {
    return meta::format_as<MetaMessageT>(message);
}

}
