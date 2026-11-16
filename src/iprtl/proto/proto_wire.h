#pragma once
#include <cstdint>
#include <variant>
#include <vector>


#include "iprtl/enums.h"
#include "iprtl/proto/wire/length.h"
#include "iprtl/proto/wire/read.h"
#include "iprtl/proto/wire/types.h"
#include "iprtl/proto/wire/write.h"
#include "iprtl/proto/wire/is_correct_length.h"

namespace iprtl::proto::wire {


inline std::tuple<std::span<const uint8_t>, CheckResult> is_correct_record(std::span<const uint8_t> buffer){
    auto [_,correct_length] = is_correct_varint(buffer);
    if (correct_length == CheckResult::Ok) {
        auto [remaining, tag] = read_varint(buffer);
        auto [type, _ ] = parse_tag(tag);
        switch(type) {
            case wire::type_id::VARINT: return is_correct_varint(remaining);
            case wire::type_id::I32: return is_correct_i32(remaining);
            case wire::type_id::I64: return is_correct_i64(remaining);
            case wire::type_id::LEN: return is_correct_len_data(remaining);
            default: return {buffer, CheckResult::Expected_Tag};
        }
    }
    return {buffer, correct_length};
}


inline std::optional<record> find_field(std::span<const uint8_t> buffer, uint32_t field_number) {

    while(not buffer.empty()) {
        record record;
        std::tie(buffer, record) = read_record(buffer);
        if (record.field_number == field_number) {
            return record;
        }
    }
    return std::nullopt;
}
} // namespace iprtl::proto::wire

