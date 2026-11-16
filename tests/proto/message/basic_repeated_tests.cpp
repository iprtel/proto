#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <span>

#include "basic_repeated.proto.h"

struct BasicTestParam {
    tests::BasicRepeated message;
    std::vector<uint8_t> encoded;
};

struct BasicRepeatedTest : public ::testing::TestWithParam<BasicTestParam> {};

TEST_P(BasicRepeatedTest, length) {
    const auto& param = GetParam();
    auto length = encoded_length(param.message);
    EXPECT_EQ(length, param.encoded.size()) << fmt::format("{}", param.message);
}

TEST_P(BasicRepeatedTest, encode_message) {
    const auto& param = GetParam();
    auto encoded_message = encode_message(param.message);
    EXPECT_EQ(encoded_message.size(), param.encoded.size()) << fmt::format("{}", param.message);
    EXPECT_THAT(encoded_message, testing::ElementsAreArray( param.encoded));
}
TEST_P(BasicRepeatedTest, decode_message) {
    const auto& param = GetParam();
    tests::BasicRepeated message{};
    auto remaining = decode_message(message, {param.encoded});
    EXPECT_EQ(message, param.message) << fmt::format("{} != {}", message, param.message);
}



INSTANTIATE_TEST_SUITE_P(
RoundTrip,
BasicRepeatedTest,
::testing::Values(
    BasicTestParam{{}, {}},
    BasicTestParam{{.a_repeated={-1,0,1}}, {0x08, 0x01, 0x08, 0x00, 0x08, 0x02}},
    BasicTestParam{{.a_strings={"a","b","c"}}, {0x12, 0x01, 0x61, 0x12, 0x01, 0x62, 0x12, 0x01, 0x63}},
    BasicTestParam{{.a_packed={-1,0,1}}, {0x1a, 0x03, 0x01, 0x00, 0x02}}
)
);

// A parser must accept a packable repeated field in either encoding, whatever
// the field's [packed] option says: packed fields may arrive as one record per
// element, and unpacked fields may arrive as a single packed record. Records of
// both kinds are concatenated in order. Encoders still write the declared form,
// so these cases are decode-only.
// See https://protobuf.dev/programming-guides/encoding/#packed
struct BasicRepeatedAlternateEncodingTest : public ::testing::TestWithParam<BasicTestParam> {};

TEST_P(BasicRepeatedAlternateEncodingTest, decode_message) {
    const auto& param = GetParam();
    tests::BasicRepeated message{};
    decode_message(message, {param.encoded});
    EXPECT_EQ(message, param.message) << fmt::format("{} != {}", message, param.message);
}

INSTANTIATE_TEST_SUITE_P(
AlternateEncoding,
BasicRepeatedAlternateEncodingTest,
::testing::Values(
    // a_packed (field 3, packed) sent unpacked: one varint record (tag 0x18) per element.
    BasicTestParam{{.a_packed={-1,0,1}}, {0x18, 0x01, 0x18, 0x00, 0x18, 0x02}},
    // a_repeated (field 1, packed=false) sent packed: one len record (tag 0x0a).
    BasicTestParam{{.a_repeated={-1,0,1}}, {0x0a, 0x03, 0x01, 0x00, 0x02}},
    // Both forms mixed in one message are appended in order.
    BasicTestParam{{.a_packed={-1,0,1}}, {0x1a, 0x02, 0x01, 0x00, 0x18, 0x02}},
    BasicTestParam{{.a_repeated={-1,0,1}}, {0x08, 0x01, 0x0a, 0x02, 0x00, 0x02}}
)
);
