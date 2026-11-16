#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <span>

#include "basic_floats.proto.h"
#include "basic_strings.proto.h"

TEST(DecodeErrorTests, decode_error) {
    std::array<uint8_t,5> encoded = {0x08, 0x00, 0x00, 0x80, 0x3f};
    tests::BasicFloats message{};
    EXPECT_THROW(decode_message(message, {encoded}), iprtl::proto::wire_exception);
}

TEST(DecodeErrorTests, decode_error_2) {
    std::array<uint8_t,4> encoded = {0x0d, 0x00, 0x80, 0x3f};
    tests::BasicFloats message{};
    EXPECT_THROW(decode_message(message, {encoded}), iprtl::proto::wire_exception);
}

TEST(DecodeErrorTests, decode_error_3) {
    std::array<uint8_t,5> encoded = {0x68, 0x03, 0x61, 0x62, 0x63};
    tests::BasicStrings message{};
    EXPECT_THROW(decode_message(message, {encoded}), iprtl::proto::wire_exception);
}

TEST(DecodeErrorTests, decode_error_4) {
    std::array<uint8_t,4> encoded = {0x6a, 0x03, 0x61, 0x62};
    tests::BasicStrings message{};
    EXPECT_THROW(decode_message(message, {encoded}), iprtl::proto::wire_exception);
}
