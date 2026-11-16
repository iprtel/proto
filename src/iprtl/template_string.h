#pragma once
#include <cstdint>

namespace iprtl {
// A helper to wrap a string literal into a compile-time object
template <std::size_t N>
struct TemplateString {
    char value[N];
    std::size_t size{N};

    constexpr TemplateString(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }

    constexpr bool operator==(const TemplateString&) const = default;
};
}