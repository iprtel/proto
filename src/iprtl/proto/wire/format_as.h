#pragma once

#include <fmt/core.h>

#include "iprtl/proto/wire/types.h"

namespace iprtl::proto::wire {

inline auto format_as(varint value)  {
    return fmt::format("");
}

inline auto format_as(i32 value)  {
    return std::to_string(value.content);
}

inline auto format_as(i64 value)  {
    return std::to_string(value.content);
}

inline auto format_as(len_data value)  {
    return fmt::format("");
}

}
