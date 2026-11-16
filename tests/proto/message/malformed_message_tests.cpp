// Decoding buffers that are not valid protobuf messages.
//
// Field layout of the test messages (tag byte = field_number << 3 | wire_type):
//   BasicInts:     a int32 = 1 (0x08), b sint32 = 2 (0x10), d sfixed32 = 4 (0x25)
//   BasicFloats:   a float = 1 (0x0d), b double = 2 (0x11)
//   BasicStrings:  text string = 13 (0x6a)
//   BasicNested:   nested Nested = 1 (0x0a);  Nested: value sint32 = 1 (0x08)
//   BasicRepeated: a_repeated unpacked sint32 = 1 (0x08), a_strings string = 2 (0x12),
//                  a_packed packed sint32 = 3 (0x1a)

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

#include "basic_floats.proto.h"
#include "basic_ints.proto.h"
#include "basic_nested.proto.h"
#include "basic_repeated.proto.h"
#include "basic_strings.proto.h"

using iprtl::proto::wire::wire_exception;

namespace {

template <typename MessageT>
void decode_bytes(MessageT& message, std::vector<uint8_t> const& bytes) {
    decode_message(message, std::span<const uint8_t>(bytes));
}

template <typename MessageT>
void expect_wire_exception(std::vector<uint8_t> const& bytes) {
    MessageT message{};
    EXPECT_THROW(decode_bytes(message, bytes), wire_exception);
}

// Varint encoding of a 64-bit value, for tags that do not fit in one byte.
std::vector<uint8_t> varint_bytes(uint64_t value) {
    std::vector<uint8_t> bytes;
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0) byte |= 0x80;
        bytes.push_back(byte);
    } while (value != 0);
    return bytes;
}

std::vector<uint8_t> concat(std::initializer_list<std::vector<uint8_t>> parts) {
    std::vector<uint8_t> result;
    for (auto const& part : parts) result.insert(result.end(), part.begin(), part.end());
    return result;
}

} // namespace

// ---------------------------------------------------------------------------
// Buffer ends in the middle of a record
// ---------------------------------------------------------------------------

TEST(TruncatedInput, tag_without_value) {
    expect_wire_exception<tests::BasicInts>({0x08});
}

TEST(TruncatedInput, truncated_tag) {
    expect_wire_exception<tests::BasicInts>({0x80});
}

TEST(TruncatedInput, truncated_varint_value) {
    // 0x96 has the continuation bit set but no byte follows.
    expect_wire_exception<tests::BasicInts>({0x08, 0x96});
}

TEST(TruncatedInput, truncated_fixed32) {
    expect_wire_exception<tests::BasicInts>({0x25, 0x01, 0x02, 0x03});
}

TEST(TruncatedInput, truncated_fixed64) {
    expect_wire_exception<tests::BasicFloats>({0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
}

TEST(TruncatedInput, truncated_length_prefix) {
    expect_wire_exception<tests::BasicStrings>({0x6a, 0x80});
}

TEST(TruncatedInput, length_longer_than_buffer) {
    expect_wire_exception<tests::BasicStrings>({0x6a, 0x05, 'a', 'b'});
}

TEST(TruncatedInput, huge_length) {
    // Length prefix is 2^64 - 1; must fail cleanly instead of allocating.
    expect_wire_exception<tests::BasicStrings>(
        {0x6a, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01});
}

TEST(TruncatedInput, valid_field_followed_by_truncated_field) {
    expect_wire_exception<tests::BasicInts>({0x08, 0x01, 0x10});
}

// ---------------------------------------------------------------------------
// Varints longer than 10 bytes
// ---------------------------------------------------------------------------

TEST(InvalidVarint, value_longer_than_10_bytes) {
    expect_wire_exception<tests::BasicInts>(
        {0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01});
}

TEST(InvalidVarint, tag_longer_than_10_bytes) {
    expect_wire_exception<tests::BasicInts>(
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01});
}

// ---------------------------------------------------------------------------
// Wire types the library does not support (groups) or that do not exist
// ---------------------------------------------------------------------------

TEST(InvalidWireType, rejects_groups_and_undefined_types) {
    for (uint8_t wire_type : {3, 4, 6, 7}) {
        SCOPED_TRACE(testing::Message() << "wire type " << int(wire_type));
        uint8_t tag = (1 << 3) | wire_type;
        expect_wire_exception<tests::BasicInts>({tag, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    }
}

// ---------------------------------------------------------------------------
// Known field arrives with a different wire type than its declaration
// ---------------------------------------------------------------------------

TEST(WireTypeMismatch, int32_as_len) {
    expect_wire_exception<tests::BasicInts>({0x0a, 0x01, 0x00});
}

TEST(WireTypeMismatch, int32_as_i32) {
    expect_wire_exception<tests::BasicInts>({0x0d, 0x00, 0x00, 0x00, 0x00});
}

TEST(WireTypeMismatch, sfixed32_as_varint) {
    expect_wire_exception<tests::BasicInts>({0x20, 0x01});
}

TEST(WireTypeMismatch, double_as_i32) {
    expect_wire_exception<tests::BasicFloats>({0x15, 0x00, 0x00, 0x80, 0x3f});
}

TEST(WireTypeMismatch, string_as_varint) {
    expect_wire_exception<tests::BasicStrings>({0x68, 0x01});
}

TEST(WireTypeMismatch, nested_message_as_varint) {
    expect_wire_exception<tests::BasicNested>({0x08, 0x01});
}

// Repeated fields must reject a record with the wrong wire type, not drop it.
TEST(WireTypeMismatch, repeated_string_as_varint) {
    expect_wire_exception<tests::BasicRepeated>({0x10, 0x01});
}

// Repeated fields must reject a record with the wrong wire type, not drop it.
TEST(WireTypeMismatch, unpacked_repeated_as_i32) {
    expect_wire_exception<tests::BasicRepeated>({0x0d, 0x00, 0x00, 0x00, 0x00});
}

// Must throw wire_exception, not std::bad_variant_access.
TEST(WireTypeMismatch, packed_repeated_as_i32) {
    expect_wire_exception<tests::BasicRepeated>({0x1d, 0x00, 0x00, 0x00, 0x00});
}

// ---------------------------------------------------------------------------
// Errors inside a nested message
// ---------------------------------------------------------------------------

TEST(MalformedNested, length_longer_than_buffer) {
    expect_wire_exception<tests::BasicNested>({0x0a, 0x05, 0x08, 0x02});
}

TEST(MalformedNested, truncated_inner_value) {
    expect_wire_exception<tests::BasicNested>({0x0a, 0x02, 0x08, 0x80});
}

TEST(MalformedNested, inner_record_crosses_nested_boundary) {
    // The nested message is 1 byte long and holds only a tag. The value byte
    // (0x02) follows outside the nested message and must not be used.
    expect_wire_exception<tests::BasicNested>({0x0a, 0x01, 0x08, 0x02});
}

TEST(MalformedNested, inner_invalid_wire_type) {
    expect_wire_exception<tests::BasicNested>({0x0a, 0x02, 0x0b, 0x00});
}

TEST(MalformedNested, inner_wire_type_mismatch) {
    expect_wire_exception<tests::BasicNested>({0x0a, 0x05, 0x0d, 0x00, 0x00, 0x00, 0x00});
}

// ---------------------------------------------------------------------------
// Errors inside a packed repeated field
// ---------------------------------------------------------------------------

TEST(MalformedPacked, truncated_element) {
    expect_wire_exception<tests::BasicRepeated>({0x1a, 0x02, 0x02, 0x80});
}

TEST(MalformedPacked, length_longer_than_buffer) {
    expect_wire_exception<tests::BasicRepeated>({0x1a, 0x05, 0x02});
}

TEST(MalformedPacked, element_longer_than_10_bytes) {
    expect_wire_exception<tests::BasicRepeated>(
        {0x1a, 0x0b, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01});
}

// ---------------------------------------------------------------------------
// Fields the message does not declare
// ---------------------------------------------------------------------------

TEST(UnknownFields, well_formed_unknown_field_is_skipped) {
    // Field 6 is not part of BasicInts.
    tests::BasicInts message{};
    decode_bytes(message, {0x30, 0x05, 0x08, 0x01});
    EXPECT_EQ(message.a, 1);
}

TEST(UnknownFields, truncated_unknown_field) {
    expect_wire_exception<tests::BasicInts>({0x30, 0x80});
}

TEST(UnknownFields, unknown_field_length_longer_than_buffer) {
    expect_wire_exception<tests::BasicInts>({0x32, 0x05, 0x01});
}

TEST(UnknownFields, unknown_field_with_invalid_wire_type) {
    expect_wire_exception<tests::BasicInts>({0x37, 0x00});
}

// ---------------------------------------------------------------------------
// Field numbers outside the valid range 1 .. 2^29 - 1
// ---------------------------------------------------------------------------

TEST(InvalidFieldNumber, largest_valid_field_number_is_accepted) {
    tests::BasicInts message{};
    auto tag = varint_bytes(((uint64_t{1} << 29) - 1) << 3);
    decode_bytes(message, concat({tag, {0x05}, {0x08, 0x01}}));
    EXPECT_EQ(message.a, 1);
}

// Field number 0 is reserved and must be rejected, not skipped as unknown.
TEST(InvalidFieldNumber, field_number_zero) {
    expect_wire_exception<tests::BasicInts>({0x00, 0x01});
}

// Field number 2^29 is one past the maximum and must be rejected, not skipped as unknown.
TEST(InvalidFieldNumber, field_number_too_large) {
    auto tag = varint_bytes(uint64_t{1} << 29 << 3);
    expect_wire_exception<tests::BasicInts>(concat({tag, {0x01}}));
}

// parse_tag must keep all bits of the field number. Truncated to 32 bits, it would decode into field 1.
TEST(InvalidFieldNumber, field_number_must_not_alias_a_declared_field) {
    // Field number 2^32 + 1 must not be read as field 1.
    auto tag = varint_bytes(((uint64_t{1} << 32) + 1) << 3);
    tests::BasicInts message{};
    EXPECT_THROW(decode_bytes(message, concat({tag, {0x05}})), wire_exception);
    EXPECT_EQ(message.a, 0);
}
