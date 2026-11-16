#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iprtl/proto/proto_wire.h"
#include <span>


using namespace iprtl::proto::wire;

template<typename T>
struct FixedTestParam{
    T expected_decoded;
    std::array<uint8_t, sizeof(T)> expected_encoded;
};

class Fixed32Test : public ::testing::TestWithParam<FixedTestParam<i32>> {
};

TEST_P(Fixed32Test, write) {
    const auto& param = GetParam();
    std::array<uint8_t, 10> buffer{};
    auto remaining = write_i32(std::span<uint8_t>(buffer.data(), buffer.size()), i32{param.expected_decoded});
    auto written_size = buffer.size() - remaining.size();
    EXPECT_THAT(std::span<uint8_t>(buffer.data(), written_size),
         testing::ElementsAreArray( param.expected_encoded));
}
TEST_P(Fixed32Test, read) {
    const auto& param = GetParam();
    auto [decoded_remaining, decoded_value] = read_i32(param.expected_encoded);
    EXPECT_EQ(decoded_value, param.expected_decoded);
}

INSTANTIATE_TEST_SUITE_P(
MyInstantiation, // Name der Instanzierung
Fixed32Test, // Name der Test-Fixture-Klasse
::testing::Values(
    FixedTestParam<i32>{i32{0u}, {0x00, 0x00, 0x00, 0x00}},
    FixedTestParam<i32>{i32{1u}, {0x01, 0x00, 0x00, 0x00}},
    FixedTestParam<i32>{i32{300u}, {0x2C, 0x01, 0x00, 0x00}},
    FixedTestParam<i32>{i32{127u}, {0x7F, 0x00, 0x00, 0x00}},
    FixedTestParam<i32>{i32{128u}, {0x80, 0x00, 0x00, 0x00}},
    FixedTestParam<i32>{i32{std::numeric_limits<uint32_t>::max()}, {0xFF, 0xFF, 0xFF, 0xFF}}
) // Parameterwerte
);


class Fixed64Test : public ::testing::TestWithParam<FixedTestParam<i64>> {
};

TEST_P(Fixed64Test, write) {
    const auto& param = GetParam();
    std::array<uint8_t, 10> buffer{};
    auto remaining = write_i64(std::span<uint8_t>(buffer.data(), buffer.size()), i64{param.expected_decoded});
    auto written_size = buffer.size() - remaining.size();
    EXPECT_THAT(std::span<uint8_t>(buffer.data(), written_size),
         testing::ElementsAreArray( param.expected_encoded));
}
TEST_P(Fixed64Test, read) {
    const auto& param = GetParam();
    auto [decoded_remaining, decoded_value] = read_i64(param.expected_encoded);
    EXPECT_EQ(decoded_value, param.expected_decoded);
}

INSTANTIATE_TEST_SUITE_P(
MyInstantiation, // Name der Instanzierung
Fixed64Test, // Name der Test-Fixture-Klasse
::testing::Values(
    FixedTestParam<i64>{i64{0u}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    FixedTestParam<i64>{i64{1u}, {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    FixedTestParam<i64>{i64{300u}, {0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    FixedTestParam<i64>{i64{127u}, {0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    FixedTestParam<i64>{i64{128u}, {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    FixedTestParam<i64>{i64{std::numeric_limits<uint64_t>::max()}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}
) // Parameterwerte
);

TEST(FixedReadErrors, truncated_i32_throws) {
    std::vector<uint8_t> encoded{0x00, 0x80, 0x3f};
    EXPECT_THROW(read_i32(encoded), iprtl::proto::wire::wire_exception);
}

TEST(FixedReadErrors, truncated_i64_throws) {
    std::vector<uint8_t> encoded{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    EXPECT_THROW(read_i64(encoded), iprtl::proto::wire::wire_exception);
}

TEST(FixedReadErrors, empty_buffer_throws) {
    std::vector<uint8_t> encoded{};
    EXPECT_THROW(read_i32(encoded), iprtl::proto::wire::wire_exception);
    EXPECT_THROW(read_i64(encoded), iprtl::proto::wire::wire_exception);
}
