#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <span>

#include "basic_ints.proto.h"

struct BasicTestParam {
    tests::BasicInts message;
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
    tests::BasicInts message{};
    auto remaining = decode_message(message, {param.encoded});
    EXPECT_EQ(message, param.message) << fmt::format("{} != {}", message, param.message);
}



INSTANTIATE_TEST_SUITE_P(
MyInstantiation, // Name der Instanzierung
BasicStringsTest, // Name der Test-Fixture-Klasse
::testing::Values(
    BasicTestParam{{}, {}},
    BasicTestParam{{.a=1}, {0x08, 0x01}},
    BasicTestParam{{.a=-1}, {0x08, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01}},
    BasicTestParam{{.b=1}, {0x10, 0x02}},
    BasicTestParam{{.b=-1}, {0x10, 0x01}},
    BasicTestParam{{.c=1}, {0x18, 0x01}},
    BasicTestParam{{.c=static_cast<decltype(tests::BasicInts::c)>(-1)}, {0x18, 0xff, 0xff, 0xff, 0xff, 0x0f}},
    BasicTestParam{{.d=1}, {0x25, 0x01, 0x00, 0x00, 0x00}},
    BasicTestParam{{.d=-1}, {0x25, 0xff, 0xff, 0xff, 0xff}},
    BasicTestParam{{.e=1}, {0x2d, 0x01, 0x00, 0x00, 0x00}},
    BasicTestParam{{.e=static_cast<decltype(tests::BasicInts::e)>(-1)}, {0x2d, 0xff, 0xff, 0xff, 0xff}},
    BasicTestParam{{.a64=1}, {0x58, 0x01}},
    BasicTestParam{{.a64=-1}, {0x58, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01}},
    BasicTestParam{{.b64=1}, {0x60, 0x02}},
    BasicTestParam{{.b64=-1}, {0x60, 0x01}}

) // Parameterwerte
);