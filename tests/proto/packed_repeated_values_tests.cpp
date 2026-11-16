#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <limits>
#include <span>
#include <vector>

#include "iprtl/proto/packed_codec.h"

using namespace iprtl::proto;

namespace {

enum class Color : int32_t {
    COLOR_UNSPECIFIED = 0,
    COLOR_RED = 1,
    COLOR_GREEN = 2,
    COLOR_BLUE = 3,
    COLOR_NEGATIVE = -1,
};

using color_p = enum_p<Color>;

template <typename T>
using values_of = std::vector<typename T::cpp_type>;

// Decode the payload of a packed field (without the length prefix).
template <typename T>
values_of<T> decode_packed(const std::vector<uint8_t>& payload) {
    packed_repeated_values<T> package;
    package.data.content = payload;
    values_of<T> result;
    for (auto value : package) {
        result.push_back(value);
    }
    return result;
}

// Check that the payload decodes to the expected values.
template <typename T>
void expect_decodes(const std::vector<uint8_t>& payload, const values_of<T>& expected) {
    EXPECT_EQ(decode_packed<T>(payload), expected);
}

// Encode values as a packed field, check the length prefix and the predicted
// length, and decode the payload again.
template <typename T>
void expect_round_trip(const values_of<T>& values) {
    std::vector<uint8_t> buffer(4096);
    auto remaining = encode<packed_p<T>>(std::span(buffer), values);
    auto written = buffer.size() - remaining.size();

    EXPECT_EQ(length<packed_p<T>>(values), written) << "predicted length differs from encoded length";

    auto [payload, prefix] = wire::read_varint(std::span<const uint8_t>(buffer.data(), written));
    ASSERT_EQ(static_cast<uint64_t>(prefix), payload.size()) << "length prefix must count payload bytes";

    EXPECT_EQ(decode_packed<T>({payload.begin(), payload.end()}), values);
}

} // namespace

// ---------------------------------------------------------------------------
// Varint-encoded types
// ---------------------------------------------------------------------------

TEST(PackedRepeatedValues, int32) {
    // Example from the protobuf encoding documentation.
    expect_decodes<int32_p>({0x03, 0x8E, 0x02, 0x9E, 0xA7, 0x05}, {3, 270, 86942});
    // Negative int32 values are sign-extended to 10 bytes.
    expect_decodes<int32_p>({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}, {-1});
    expect_round_trip<int32_p>({0, 1, -1, 127, 128, std::numeric_limits<int32_t>::max(),
                                std::numeric_limits<int32_t>::min()});
}

TEST(PackedRepeatedValues, int64) {
    expect_decodes<int64_p>({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F},
                            {std::numeric_limits<int64_t>::max()});
    expect_decodes<int64_p>({0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01},
                            {std::numeric_limits<int64_t>::min()});
    expect_round_trip<int64_p>({0, 1, -1, std::numeric_limits<int64_t>::max(),
                                std::numeric_limits<int64_t>::min()});
}

TEST(PackedRepeatedValues, uint32) {
    expect_decodes<uint32_p>({0x00, 0x7F, 0x80, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F},
                             {0, 127, 128, std::numeric_limits<uint32_t>::max()});
    expect_round_trip<uint32_p>({0, 1, 300, std::numeric_limits<uint32_t>::max()});
}

TEST(PackedRepeatedValues, uint64) {
    expect_decodes<uint64_p>({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01},
                             {std::numeric_limits<uint64_t>::max()});
    expect_round_trip<uint64_p>({0, 1, 16384, std::numeric_limits<uint64_t>::max()});
}

TEST(PackedRepeatedValues, sint32) {
    // ZigZag: 0 -> 0, -1 -> 1, 1 -> 2, -2 -> 3
    expect_decodes<sint32_p>({0x00, 0x01, 0x02, 0x03}, {0, -1, 1, -2});
    expect_decodes<sint32_p>({0xFE, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F},
                             {std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min()});
    expect_round_trip<sint32_p>({0, -1, 1, -64, 64, std::numeric_limits<int32_t>::max(),
                                 std::numeric_limits<int32_t>::min()});
}

TEST(PackedRepeatedValues, sint64) {
    expect_decodes<sint64_p>({0x01, 0x02}, {-1, 1});
    expect_decodes<sint64_p>({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01},
                             {std::numeric_limits<int64_t>::min()});
    expect_round_trip<sint64_p>({0, -1, 1, std::numeric_limits<int64_t>::max(),
                                 std::numeric_limits<int64_t>::min()});
}

TEST(PackedRepeatedValues, bool) {
    expect_decodes<bool_p>({0x01, 0x00, 0x01}, {true, false, true});
    expect_round_trip<bool_p>({true, true, false, true, false, false});
}

TEST(PackedRepeatedValues, enum) {
    expect_decodes<color_p>({0x03, 0x00, 0x02}, {Color::COLOR_BLUE, Color::COLOR_UNSPECIFIED, Color::COLOR_GREEN});
    // Negative enum values are encoded like negative int32 values (10 bytes).
    expect_decodes<color_p>({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}, {Color::COLOR_NEGATIVE});
    expect_round_trip<color_p>({Color::COLOR_RED, Color::COLOR_GREEN, Color::COLOR_BLUE,
                                Color::COLOR_UNSPECIFIED, Color::COLOR_NEGATIVE});
}

// ---------------------------------------------------------------------------
// Fixed 32-bit types
// ---------------------------------------------------------------------------

TEST(PackedRepeatedValues, fixed32) {
    expect_decodes<fixed32_p>({0x01, 0x00, 0x00, 0x00, 0xEF, 0xBE, 0xAD, 0xDE}, {1, 0xDEADBEEF});
    expect_round_trip<fixed32_p>({0, 1, std::numeric_limits<uint32_t>::max()});
}

TEST(PackedRepeatedValues, sfixed32) {
    expect_decodes<sfixed32_p>({0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x80},
                               {-1, std::numeric_limits<int32_t>::min()});
    expect_round_trip<sfixed32_p>({0, -1, 1, std::numeric_limits<int32_t>::max(),
                                   std::numeric_limits<int32_t>::min()});
}

TEST(PackedRepeatedValues, float) {
    // 1.0f = 0x3F800000, -2.5f = 0xC0200000 (little endian on the wire)
    expect_decodes<float_p>({0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x20, 0xC0}, {1.0f, -2.5f});
    expect_round_trip<float_p>({0.0f, 1.0f, -2.5f, 12.375f, std::numeric_limits<float>::max(),
                                std::numeric_limits<float>::lowest(), std::numeric_limits<float>::infinity()});
}

// ---------------------------------------------------------------------------
// Fixed 64-bit types
// ---------------------------------------------------------------------------

TEST(PackedRepeatedValues, fixed64) {
    expect_decodes<fixed64_p>({0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                              {1, std::numeric_limits<uint64_t>::max()});
    expect_round_trip<fixed64_p>({0, 1, std::numeric_limits<uint64_t>::max()});
}

TEST(PackedRepeatedValues, sfixed64) {
    expect_decodes<sfixed64_p>({0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {-2});
    expect_round_trip<sfixed64_p>({0, -1, 1, std::numeric_limits<int64_t>::max(),
                                   std::numeric_limits<int64_t>::min()});
}

TEST(PackedRepeatedValues, double) {
    // 1.0 = 0x3FF0000000000000, -0.5 = 0xBFE0000000000000
    expect_decodes<double_p>({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xBF},
                             {1.0, -0.5});
    expect_round_trip<double_p>({0.0, 1.0, -0.5, 3.141592653589793, std::numeric_limits<double>::max(),
                                 std::numeric_limits<double>::lowest(), -std::numeric_limits<double>::infinity()});
}

// ---------------------------------------------------------------------------
// Empty payloads, many elements, and malformed data
// ---------------------------------------------------------------------------

TEST(PackedRepeatedValues, empty_payload_has_no_elements) {
    EXPECT_TRUE(decode_packed<int32_p>({}).empty());
    EXPECT_TRUE(decode_packed<sint64_p>({}).empty());
    EXPECT_TRUE(decode_packed<bool_p>({}).empty());
    EXPECT_TRUE(decode_packed<color_p>({}).empty());
    EXPECT_TRUE(decode_packed<fixed32_p>({}).empty());
    EXPECT_TRUE(decode_packed<float_p>({}).empty());
    EXPECT_TRUE(decode_packed<sfixed64_p>({}).empty());
    EXPECT_TRUE(decode_packed<double_p>({}).empty());
}

TEST(PackedRepeatedValues, empty_list_round_trip) {
    expect_round_trip<int32_p>({});
    expect_round_trip<double_p>({});
}

TEST(PackedRepeatedValues, zero_values_are_kept) {
    // A zero element is a valid entry and must not end the iteration early.
    expect_decodes<int32_p>({0x00, 0x00, 0x00}, {0, 0, 0});
    expect_decodes<fixed32_p>({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0, 0});
}

TEST(PackedRepeatedValues, repeated_equal_values_are_kept) {
    expect_decodes<uint32_p>({0x05, 0x05, 0x05}, {5, 5, 5});
}

// The length prefix counts payload bytes, not elements. 32 fixed32 values are
// 128 bytes, so the prefix needs 2 bytes, although 32 alone would fit in 1.
TEST(PackedRepeatedValues, length_prefix_counts_bytes_not_elements) {
    values_of<fixed32_p> values(32, 0x01020304u);
    expect_round_trip<fixed32_p>(values);
    EXPECT_EQ(length<packed_p<fixed32_p>>(values), 2u + 128u);
}

TEST(PackedRepeatedValues, many_varint_elements) {
    values_of<sint32_p> values;
    for (int32_t i = -500; i <= 500; ++i) {
        values.push_back(i * 1000);
    }
    expect_round_trip<sint32_p>(values);
}

TEST(PackedRepeatedValues, truncated_varint_throws) {
    EXPECT_THROW(decode_packed<int32_p>({0x01, 0x80}), wire::wire_exception);
}

TEST(PackedRepeatedValues, truncated_fixed32_throws) {
    // One complete float followed by one byte too few for the next.
    EXPECT_THROW(decode_packed<float_p>({0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80}), wire::wire_exception);
    EXPECT_THROW(decode_packed<fixed32_p>({0x01, 0x00}), wire::wire_exception);
}

TEST(PackedRepeatedValues, truncated_fixed64_throws) {
    EXPECT_THROW(decode_packed<double_p>({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0}), wire::wire_exception);
    EXPECT_THROW(decode_packed<sfixed64_p>({0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}), wire::wire_exception);
}
