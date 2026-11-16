#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <span>

#include "basic_strings.proto.h"

struct BasicTestParam {
    tests::BasicStrings message;
    std::vector<uint8_t> encoded;
};

struct BasicStringsTest : public ::testing::TestWithParam<BasicTestParam> {};

TEST_P(BasicStringsTest, length) {
    const auto& param = GetParam();
    auto length = encoded_length(param.message);
    EXPECT_EQ(length, param.encoded.size()) << fmt::format("{}", param.message);
}

TEST_P(BasicStringsTest, encode_message) {
    const auto& param = GetParam();
    auto encoded_message = encode_message(param.message);
    EXPECT_EQ(encoded_message.size(), param.encoded.size()) << fmt::format("{}", param.message);
    EXPECT_THAT(encoded_message, testing::ElementsAreArray( param.encoded));
}
TEST_P(BasicStringsTest, decode_message) {
    const auto& param = GetParam();
    tests::BasicStrings message{};
    auto remaining = decode_message(message, {param.encoded});
    EXPECT_EQ(message, param.message) << fmt::format("{} != {}", message, param.message);
}


INSTANTIATE_TEST_SUITE_P(
MyInstantiation, // Name der Instanzierung
BasicStringsTest, // Name der Test-Fixture-Klasse
::testing::Values(
    BasicTestParam{{}, {}},
    BasicTestParam{{.text=""}, {}},
    BasicTestParam{{.text="abc"}, {0x6a, 0x03, 0x61, 0x62, 0x63}}
) // Parameterwerte
);