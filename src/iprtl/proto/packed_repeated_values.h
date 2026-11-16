#pragma once

#include <iterator>
#include <optional>
#include <span>

#include "decode.h"
#include "wire/read.h"

namespace iprtl::proto {
template <typename T>
class packed_iterator {
public:
using iterator_category = std::forward_iterator_tag;
using value_type = T;
using difference_type = std::ptrdiff_t;
using reference = T::cpp_type&;

packed_iterator(std::span<const uint8_t> span) : m_span(span) { next_value = get_next_value(); }

T::cpp_type operator*() const {
    return next_value.value();
}

// Prefix increment
packed_iterator& operator++() {
     next_value = get_next_value();
     return *this;
}

// Postfix increment
packed_iterator operator++(int) { packed_iterator tmp = *this; ++(*this); return tmp; }

friend bool operator==(const packed_iterator& a, const packed_iterator& b) { return a.m_span.empty() == b.m_span.empty() && a.next_value == b.next_value; }
friend bool operator!=(const packed_iterator& a, const packed_iterator& b) { return a.m_span.empty() != b.m_span.empty() || a.next_value != b.next_value; }

private:
     std::optional<typename T::cpp_type> get_next_value() {
        if (m_span.empty()) {
            return std::nullopt;
        }
         if constexpr (std::is_same_v<typename T::wire_type, wire::varint>) {
             auto [remaining, value] = wire::read_varint(m_span);
             m_span = remaining;
             return as<T>(value);
         } else if constexpr (std::is_same_v<typename T::wire_type, wire::i32>) {
             auto [remaining, value] = wire::read_i32(m_span);
             m_span = remaining;
             return as<T>(value);
         } else if constexpr (std::is_same_v<typename T::wire_type, wire::i64>) {
             auto [remaining, value] = wire::read_i64(m_span);
             m_span = remaining;
             return as<T>(value);
         } else if constexpr (std::is_same_v<typename T::wire_type, wire::len_data>) {
             auto [remaining, value] = wire::read_len_data(m_span);
             m_span = remaining;
             return as<T>(value);
         }
         return std::nullopt;
     }


    std::span<const uint8_t> m_span;
    std::optional<typename T::cpp_type> next_value{};
};


template<typename ElementType>
struct packed_repeated_values {
    using element_type = ElementType;
    wire::len_data data;
    packed_iterator<ElementType> begin() {
        return packed_iterator<ElementType>(std::span<uint8_t>(data.content.data(), data.content.size()));
    }
    packed_iterator<ElementType> end() {
        return packed_iterator<ElementType>(std::span<uint8_t>());
    }
};


}