#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <span>

#include "basic_enums.proto.h"

struct BasicTestParam {
    tests::BasicEnums message;
    std::vector<uint8_t> encoded;
};

struct BasicEnumsTest : public ::testing::TestWithParam<BasicTestParam> {};

TEST_P(BasicEnumsTest, length) {
    const auto& param = GetParam();
    auto length = encoded_length(param.message);
    EXPECT_EQ(length, param.encoded.size()) << fmt::format("{}", param.message);
}

TEST_P(BasicEnumsTest, encode_message) {
    const auto& param = GetParam();
    auto encoded_message = encode_message(param.message);
    EXPECT_EQ(encoded_message.size(), param.encoded.size()) << fmt::format("{}", param.message);
    EXPECT_THAT(encoded_message, testing::ElementsAreArray( param.encoded));
}
TEST_P(BasicEnumsTest, decode_message) {
    const auto& param = GetParam();
    tests::BasicEnums message{};
    auto remaining = decode_message(message, {param.encoded});
    EXPECT_EQ(remaining.size(), 0) << fmt::format("{}", param.message);
    EXPECT_EQ(message, param.message) << fmt::format("{} != {}", message, param.message);
}


INSTANTIATE_TEST_SUITE_P(
RoundTrip,
BasicEnumsTest,
::testing::Values(
    BasicTestParam{{}, {}},
    BasicTestParam{{.choice=tests::Choice::CHOICE_NONE}, {}},
    BasicTestParam{{.choice=tests::Choice::CHOICE_POSITIVE}, {0x08, 0x0d}},
    BasicTestParam{{.choice=tests::Choice::CHOICE_NEGATIVE}, {0x08, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01}}
)
);