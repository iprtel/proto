#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <span>

#include "basic_floats.proto.h"

struct BasicTestParam {
    tests::BasicFloats message;
    std::vector<uint8_t> encoded;
};

struct BasicFloatsTest : public ::testing::TestWithParam<BasicTestParam> {};

TEST_P(BasicFloatsTest, length) {
    const auto& param = GetParam();
    auto length = encoded_length(param.message);
    EXPECT_EQ(length, param.encoded.size()) << fmt::format("{}", param.message);
}

TEST_P(BasicFloatsTest, encode_message) {
    const auto& param = GetParam();
    auto encoded_message = encode_message(param.message);
    EXPECT_EQ(encoded_message.size(), param.encoded.size()) << fmt::format("{}", param.message);
    EXPECT_THAT(encoded_message, testing::ElementsAreArray( param.encoded));
}
TEST_P(BasicFloatsTest, decode_message) {
    const auto& param = GetParam();
    tests::BasicFloats message{};
    auto remaining = decode_message(message, {param.encoded});
    EXPECT_EQ(remaining.size(), 0) << fmt::format("{}", param.message);
    EXPECT_EQ(message, param.message) << fmt::format("{} != {}", message, param.message);
}



INSTANTIATE_TEST_SUITE_P(
RoundTrip,
BasicFloatsTest,
::testing::Values(
    BasicTestParam{{}, {}},
    BasicTestParam{{.a=1.0f}, {0x0d, 0x00, 0x00, 0x80, 0x3f}},
    BasicTestParam{{.a=-1.0f}, {0x0d, 0x00, 0x00, 0x80, 0xbf}},
    BasicTestParam{{.b=1.0}, {0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f}},
    BasicTestParam{{.b=-1.0}, {0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xbf}}
)
);