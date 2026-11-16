#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <span>

#include "basic_nested.proto.h"

TEST(BasicNested, empty) {
    tests::BasicNested message{};
    std::array<uint8_t, 1024> buffer{};
    auto encoded_span = encode_message(message, std::span(buffer));
    EXPECT_EQ(encoded_span.size(), 0);

    EXPECT_EQ(encoded_length(message), 0);
}

TEST(BasicNested, not_empty) {
    tests::Nested nested_message{};
    nested_message.value = 42;
    auto encoded_length_nested = encoded_length(nested_message);
    EXPECT_EQ(encoded_length_nested, 2);

    tests::BasicNested message{};
    message.nested.value = 42;
    std::array<uint8_t, 1024> buffer{};
    auto encoded_span = encode_message(message, std::span(buffer));
    EXPECT_EQ(encoded_span.size(), 4);

    EXPECT_EQ(encoded_length(message), 4);
}
