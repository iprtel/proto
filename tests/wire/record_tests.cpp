#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iprtl/proto/proto_wire.h"
#include <span>
#include <fmt/core.h>
#include <fmt/ranges.h>

using namespace iprtl::proto::wire;

struct RecordTestParam {
    record expected_decoded;
    std::vector<uint8_t> expected_encoded;
};

class RecordTest : public ::testing::TestWithParam<RecordTestParam> {};
TEST_P(RecordTest, write) {
    const auto& param = GetParam();
    std::array<uint8_t, 100> buffer{};
    auto remaining = write_record(std::span<uint8_t>(buffer.data(), buffer.size()), param.expected_decoded);
    auto written_size = buffer.size() - remaining.size();
    auto written_data = std::span<uint8_t>(buffer.data(), written_size);
    EXPECT_THAT(written_data, testing::ElementsAreArray(param.expected_encoded))
    << fmt::format("Expected encoded: {:02X} ", fmt::join(param.expected_encoded, " "))
    << fmt::format("Actual encoded:   {:02X}", fmt::join(written_data, " "));
}
TEST_P(RecordTest, read) {
    const auto& param = GetParam();
    auto [decoded_remaining, decoded_value] = read_record(param.expected_encoded);
    EXPECT_EQ(decoded_value.field_number, param.expected_decoded.field_number);
    ASSERT_EQ(decoded_value.value.index(), param.expected_decoded.value.index());
    std::visit(overloaded{
        [&](varint v) {
            EXPECT_EQ(std::get<varint>(decoded_value.value), v);
        },
        [&](i32 v) {
            EXPECT_EQ(std::get<i32>(decoded_value.value), v);
        },
        [&](i64 v) {
            EXPECT_EQ(std::get<i64>(decoded_value.value), v);
        },
        [&](len_data v) {
            EXPECT_THAT(std::get<len_data>(decoded_value.value).content, testing::ElementsAreArray(v.content));
        }
    }, param.expected_decoded.value);
}


INSTANTIATE_TEST_SUITE_P(
MyInstantiation, // Name der Instanzierung
RecordTest, // Name der Test-Fixture-Klasse
::testing::Values(
    RecordTestParam{{1, varint{0}}, {0x08, 0x00}},
    RecordTestParam{{2, i32{42}}, {0x15, 0x2A, 0x00, 0x00, 0x00}},
    RecordTestParam{{3, i64{84}}, {0x19, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    RecordTestParam{{4, len_data{{5, 6, 7}}}, {0x22, 0x03, 5, 6, 7}},
    RecordTestParam{{128, varint{128}}, {0x80, 0x08, 0x80, 0x01}}
)
// Parameterwerte
);


std::array<uint8_t, 45> message_data = {
    0x08, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x01, 0x10, 0x03, 0x18, 0x03, 0x25, 0xfc, 0xff, 0xff, 0xff,
    0x2d, 0x05, 0x00, 0x00, 0x00, 0x32, 0x03, 0x73, 0x69, 0x78,
    0x58, 0xf5, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x01, 0x60, 0x17, 0x68, 0x0d
};
std::array<record, 9> message_records = {
        record{1, varint{18446744073709551615ULL}},
        record{2, varint{3}},
        record{3, varint{3}},
        record{4, i32{4294967292U}},
        record{5, i32{5}},
        record{6, len_data{ {'s', 'i', 'x'} }},
        record{11, varint{18446744073709551605ULL}},
        record{12, varint{23}},
        record{13, varint{13}}
};

TEST(RecordsTest, multi_read) {

    std::vector<record> decoded_records;
    std::span<const uint8_t> remaining(message_data);
    while(not remaining.empty()) {
        record record;
        std::tie(remaining, record) = read_record(remaining);
        decoded_records.push_back(record);
    }

   EXPECT_THAT(decoded_records, testing::ElementsAreArray(message_records));
}

TEST(RecordsTest, find_field) {
    auto record_opt = find_field(std::span<const uint8_t>(message_data), 6);
    ASSERT_TRUE(record_opt.has_value());
    EXPECT_EQ(record_opt->field_number, 6);
    ASSERT_EQ(record_opt->value.index(), 3); // len_data
    auto const& data = std::get<len_data>(record_opt->value);
    EXPECT_THAT(data.content, testing::ElementsAreArray(std::vector<uint8_t>{'s', 'i', 'x'}));
}

struct RecordLengthTestParam {
    CheckResult expected;
    std::vector<uint8_t> encoded;
};

class RecordLengthTest : public ::testing::TestWithParam<RecordLengthTestParam> {};
TEST_P(RecordLengthTest, write) {
    const auto& param = GetParam();
    auto[remaining, result]=is_correct_record(param.encoded);
    EXPECT_EQ(result,param.expected);
}


INSTANTIATE_TEST_SUITE_P(
MyLengthTest, // Name der Instanzierung
RecordLengthTest, // Name der Test-Fixture-Klasse
::testing::Values(
    RecordLengthTestParam{CheckResult::Ok, {0x08, 0x00}},
    RecordLengthTestParam{CheckResult::Ok, {0x15, 0x2A, 0x00, 0x00, 0x00}},
    RecordLengthTestParam{CheckResult::Ok, {0x19, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    RecordLengthTestParam{CheckResult::Ok, {0x22, 0x03, 5, 6, 7}},
    RecordLengthTestParam{CheckResult::Ok, {0x80, 0x08, 0x80, 0x01}}
)
);


TEST(RecordsTest, make_record_string) {
    auto record = make_record<len_data>(6, std::string{"six"});
    EXPECT_EQ(record.field_number, 6);
    ASSERT_EQ(record.value.index(), 3); // len_data
    auto const& data = std::get<len_data>(record.value);
    EXPECT_THAT(data.content, testing::ElementsAreArray(std::vector<uint8_t>{'s', 'i', 'x'}));
}

TEST(RecordsTest, make_record_int32) {
    auto record = make_record<i32>(5, 5);
    EXPECT_EQ(record.field_number, 5);
    ASSERT_EQ(record.value.index(), 1); // i32
    auto const& data = std::get<i32>(record.value);
    EXPECT_EQ(data.content, 5);
}


