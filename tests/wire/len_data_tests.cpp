#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iprtl/proto/wire/write.h"
#include "iprtl/proto/wire/read.h"
#include "iprtl/proto/wire/is_correct_length.h"
#include <span>


using namespace iprtl::proto::wire;

struct LenDataTestParam {
    len_data expected_decoded;
    std::vector<uint8_t> expected_encoded;
};

class LenDataTest : public ::testing::TestWithParam<LenDataTestParam> {};

TEST_P(LenDataTest, write) {
    const auto& param = GetParam();
    std::array<uint8_t, 100> buffer{};
    auto remaining = write_len_data(std::span<uint8_t>(buffer.data(), buffer.size()), param.expected_decoded);
    auto written_size = buffer.size() - remaining.size();
    auto written_data = std::span<uint8_t>(buffer.data(), written_size);
    EXPECT_THAT(written_data, testing::ElementsAreArray(param.expected_encoded));
}
TEST_P(LenDataTest, read) {
    const auto& param = GetParam();
    auto [decoded_remaining, decoded_value] = read_len_data(param.expected_encoded);
    EXPECT_THAT(decoded_value.content, testing::ElementsAreArray(param.expected_decoded.content));
}


INSTANTIATE_TEST_SUITE_P(
Examples,
LenDataTest,
::testing::Values(
    LenDataTestParam{{{0x00}}, {{0x01, 0x00}}},
    LenDataTestParam{{{0x01}}, {{0x01, 0x01}}},
    LenDataTestParam{{{0xFF}}, {{0x01, 0xFF}}},
    LenDataTestParam{{{0x80}}, {{0x01, 0x80}}},
    LenDataTestParam{{{0x00, 0x00, 0x00}}, {{0x03, 0x00, 0x00, 0x00}}}
)
);


struct LenDataLengthTestParam {
    CheckResult expected;
    std::vector<uint8_t> encoded;
};

class LenDataLengthTest : public ::testing::TestWithParam<LenDataLengthTestParam> {};

TEST_P(LenDataLengthTest, read) {
    const auto& param = GetParam();
    auto [remaining, result] = is_correct_len_data(param.encoded);
    EXPECT_EQ(result, param.expected);
}

INSTANTIATE_TEST_SUITE_P(
Examples,
LenDataLengthTest,
::testing::Values(
    LenDataLengthTestParam{CheckResult::Ok, {0x00, 0x00}},
    LenDataLengthTestParam{CheckResult::Ok, {0x01, 0x00}},
    LenDataLengthTestParam{CheckResult::Ok, {0x03, 0x00, 0x00, 0x00}},
    LenDataLengthTestParam{CheckResult::Expected_LenData, {0x83, 0x00, 0x00, 0x00}},
    LenDataLengthTestParam{CheckResult::Expected_LenData, {0x03, 0x00, 0x00}}
)
);
