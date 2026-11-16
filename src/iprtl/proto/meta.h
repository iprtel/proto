#pragma once

#include "meta/types.h"
#include "meta/types_format.h"
#include "meta/decode.h"
#include "length.h"
#include "encode.h"


namespace iprtl::proto {

template<typename MetaMessageT>
std::span<uint8_t> encode_message(typename MetaMessageT::message_type message, std::span<uint8_t> buffer) {
    auto remaining = encode<MetaMessageT>(buffer, message);
    return std::span<uint8_t>(buffer.data(), buffer.size() - remaining.size());
};
template<typename MetaMessageT>
std::vector<uint8_t> encode_message(typename MetaMessageT::message_type message) {
    auto size = length<MetaMessageT>(message);
    std::vector<uint8_t> result(size);
    auto remaining = encode<MetaMessageT>(std::span(result), message);
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

}