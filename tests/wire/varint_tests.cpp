#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iprtl/proto/proto_wire.h"
#include <span>


using namespace iprtl::proto::wire;

struct VarintTestParam {
    uint64_t expected_decoded;
    std::vector<uint8_t> expected_encoded;
};

struct VarintTest : public ::testing::TestWithParam<VarintTestParam> {};

TEST_P(VarintTest, write) {
    const auto& param = GetParam();
    std::array<uint8_t, 10> buffer{};
    auto remaining = write_varint(std::span<uint8_t>(buffer.data(), buffer.size()), varint{param.expected_decoded});
    auto written_size = buffer.size() - remaining.size();
    EXPECT_THAT(std::span<uint8_t>(buffer.data(), written_size),
         testing::ElementsAreArray( param.expected_encoded));
}
TEST_P(VarintTest, read) {
    const auto& param = GetParam();
    auto [decoded_remaining, decoded_value] = read_varint(param.expected_encoded);
    EXPECT_EQ(static_cast<uint64_t>(decoded_value), param.expected_decoded);
}

INSTANTIATE_TEST_SUITE_P(
MyInstantiation, // Name der Instanzierung
VarintTest, // Name der Test-Fixture-Klasse
::testing::Values(
    VarintTestParam{0, {0x00}},
    VarintTestParam{1, {0x01}},
    VarintTestParam{300, {0xAC, 0x02}},
    VarintTestParam{127, {0x7F}},
    VarintTestParam{128, {0x80, 0x01}},
    VarintTestParam{16384, {0x80, 0x80, 0x01}},
    VarintTestParam{std::numeric_limits<uint64_t>::max(), {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0x01}}
) // Parameterwerte
);


struct VarintLengthTestParam {
    CheckResult expected;
    std::vector<uint8_t> encoded;
};

struct VarintLengthTest : public ::testing::TestWithParam<VarintLengthTestParam> {};

TEST_P(VarintLengthTest, check) {
    const auto& param = GetParam();
    auto [remaining,result] = is_correct_varint(param.encoded);
    EXPECT_EQ(result, param.expected);
}

INSTANTIATE_TEST_SUITE_P(
MyLengthTests, // Name der Instanzierung
VarintLengthTest, // Name der Test-Fixture-Klasse
::testing::Values(
    VarintLengthTestParam{CheckResult::Ok, {0x00}},
    VarintLengthTestParam{CheckResult::Ok, {0x01}},
    VarintLengthTestParam{CheckResult::Ok, {0xAC, 0x02}},
    VarintLengthTestParam{CheckResult::Ok, {0x7F}},
    VarintLengthTestParam{CheckResult::Ok, {0x80, 0x01}},
    VarintLengthTestParam{CheckResult::Ok, {0x80, 0x80, 0x01}},
    VarintLengthTestParam{CheckResult::Ok, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0x01}},
    VarintLengthTestParam{CheckResult::Expected_Varint, {0x80}},
    VarintLengthTestParam{CheckResult::Expected_Varint, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF,0x01}}
) // Parameterwerte
);


TEST(VarintReadErrors, empty_buffer_throws) {
    std::vector<uint8_t> encoded{};
    EXPECT_THROW(read_varint(encoded), iprtl::proto::wire::wire_exception);
}

TEST(VarintReadErrors, truncated_varint_throws) {
    std::vector<uint8_t> encoded{0x80, 0x80};
    EXPECT_THROW(read_varint(encoded), iprtl::proto::wire::wire_exception);
}

TEST(VarintReadErrors, varint_longer_than_10_bytes_throws) {
    std::vector<uint8_t> encoded{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01};
    EXPECT_THROW(read_varint(encoded), iprtl::proto::wire::wire_exception);
}

TEST(VarintReadErrors, remaining_starts_after_varint) {
    std::vector<uint8_t> encoded{0xAC, 0x02, 0x42};
    auto [remaining, value] = read_varint(encoded);
    EXPECT_EQ(static_cast<uint64_t>(value), 300u);
    ASSERT_EQ(remaining.size(), 1u);
    EXPECT_EQ(remaining[0], 0x42);
}
