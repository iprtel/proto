

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <span>

#include "basic_enums.proto.h"
#include "basic_ints.proto.h"
#include "basic_strings.proto.h"
#include "basic_nested.proto.h"

TEST(MessageTest, basic_enums) {
    tests::BasicEnums message{};
    std::array<uint8_t, 1024> buffer{};
    auto encoded_span = encode_message(message, std::span(buffer));
    EXPECT_EQ(encoded_span.size(), 0);
    message.choice = tests::Choice::CHOICE_POSITIVE;
    encoded_span = encode_message(message, std::span(buffer));
    EXPECT_EQ(encoded_span.size(), 2);
    EXPECT_EQ(buffer[0], 0x08);
    EXPECT_EQ(buffer[1], 0x0D);
    message.choice = tests::Choice::CHOICE_NEGATIVE;
    encoded_span = encode_message(message, std::span(buffer));
    EXPECT_EQ(encoded_span.size(), 11);
    EXPECT_EQ(buffer[0], 0x08);
    EXPECT_EQ(buffer[1], 0xF3);
    EXPECT_EQ(buffer[2], 0xFF);

    EXPECT_EQ(encoded_length(message), 11);

    auto encoded_vector = encode_message(message);
    EXPECT_EQ(encoded_vector.size(), 11);
    EXPECT_EQ(encoded_vector[0], 0x08);
    EXPECT_EQ(encoded_vector[1], 0xF3);
    EXPECT_EQ(encoded_vector[2], 0xFF);
}

TEST(MessageTest, basic_ints) {
    tests::BasicInts message{};
    std::array<uint8_t, 1024> buffer{};
    auto encoded_span = encode_message(message, std::span(buffer));
    EXPECT_EQ(encoded_span.size(), 0);
    message.a = 42;
    encoded_span = encode_message(message, std::span(buffer));
    EXPECT_EQ(encoded_span.size(), 2);
    EXPECT_EQ(buffer[0], 0x08);
    EXPECT_EQ(buffer[1], 0x2A);
}

TEST(MessageTest, basic_strings) {
    tests::BasicStrings message{};
    std::array<uint8_t, 1024> buffer{};
    auto encoded_span = encode_message(message, std::span(buffer));
    EXPECT_EQ(encoded_span.size(), 0);
    message.text = "Hello";
    encoded_span = encode_message(message, std::span(buffer));
    EXPECT_EQ(encoded_span.size(), 7);
    EXPECT_EQ(buffer[0], 0x6A);
    EXPECT_EQ(buffer[1], 0x05);
    EXPECT_EQ(buffer[2], 'H');
    EXPECT_EQ(buffer[3], 'e');
    EXPECT_EQ(buffer[4], 'l');
    EXPECT_EQ(buffer[5], 'l');
    EXPECT_EQ(buffer[6], 'o');
}