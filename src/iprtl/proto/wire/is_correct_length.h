#pragma once

#include <tuple>

#include "iprtl/proto/wire/types.h"

namespace iprtl::proto::wire {

enum class CheckResult {
    Ok,
    Expected_Tag,
    Expected_Varint,
    Expected_I32,
    Expected_I64,
    Expected_LenData
};

inline std::tuple<std::span<const uint8_t>, CheckResult> is_correct_varint(std::span<const uint8_t> buffer)  {
    for (std::size_t i=0; i < 10; i++){
        if (buffer.size() <= i) {
            break;
        }
        auto is_last_byte = (buffer[i] & 0x80) == 0;
        if (is_last_byte){
            return {buffer.subspan(i), CheckResult::Ok};
        }
    }
    return {buffer, CheckResult::Expected_Varint};
}

inline std::tuple<std::span<const uint8_t>, CheckResult> is_correct_i32(std::span<const uint8_t> buffer)  {
    if (buffer.size() >= sizeof(i32)) {
        return {buffer.subspan(sizeof(i32)), CheckResult::Ok};
    } else {
        return {buffer, CheckResult::Expected_I32};
    }
}

inline std::tuple<std::span<const uint8_t>, CheckResult> is_correct_i64(std::span<const uint8_t> buffer)  {
    if (buffer.size() >= sizeof(i64)) {
        return {buffer.subspan(sizeof(i64)), CheckResult::Ok};
    } else {
        return {buffer, CheckResult::Expected_I64};
    }
}

inline std::tuple<std::span<const uint8_t>, CheckResult> is_correct_len_data(std::span<const uint8_t> buffer)  {
    auto  [_, correct_tag] = is_correct_varint(buffer);
    if (correct_tag == CheckResult::Ok) {
        auto  [remaining,length_value] = read_varint(buffer);
        uint64_t length = static_cast<uint64_t>(length_value);
        if (remaining.size() >= length) {
            return {remaining.subspan(length), CheckResult::Ok};
        }
        return {buffer, CheckResult::Expected_LenData};
    }
    return {buffer, correct_tag};
}


}


