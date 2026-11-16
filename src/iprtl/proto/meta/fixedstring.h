#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

namespace iprtl::proto::meta {

// A helper to wrap a string literal into a compile-time object
template <size_t N>
struct FixedString {
    std::array<char, N> value{};
    std::size_t size{N};

    // constexpr constructor to copy the literal
    constexpr FixedString(const char (&str)[N]) {
        std::copy_n(str, N, begin(value));
    }

    // Comparison operator required for NTTP
    constexpr bool operator==(const FixedString&) const = default;

    constexpr std::string_view view() {return std::string_view{value,size};}
    constexpr std::array<char, N> array() { return value;}
};

static_assert(FixedString("").size == 1);


}
