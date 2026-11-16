#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "iprtl/proto/compiler/proto_parser.h"

using namespace iprtl::proto::compiler;

namespace {

ProtoFile parse(const std::string& source) {
    std::istringstream input(source);
    return ProtoParser::parse(input);
}

const Field& field_named(const Message& msg, const std::string& name) {
    for (const auto& field : msg.fields) {
        if (field.name == name) {
            return field;
        }
    }
    throw std::runtime_error("field not found: " + name);
}

} // namespace

// ---------------------------------------------------------------------------
// File-level declarations: syntax, package, import
// ---------------------------------------------------------------------------

TEST(ProtoParserTest, EmptyInputDefaultsToProto3) {
    auto proto = parse("");
    EXPECT_EQ(proto.syntax, "proto3");
    EXPECT_EQ(proto.package, "");
    EXPECT_TRUE(proto.imports.empty());
    EXPECT_TRUE(proto.messages.empty());
    EXPECT_TRUE(proto.enums.empty());
}

TEST(ProtoParserTest, SyntaxProto3) {
    auto proto = parse(R"(syntax = "proto3";)");
    EXPECT_EQ(proto.syntax, "proto3");
}

TEST(ProtoParserTest, SyntaxProto2) {
    auto proto = parse(R"(syntax = "proto2";)");
    EXPECT_EQ(proto.syntax, "proto2");
}

TEST(ProtoParserTest, InvalidSyntaxThrows) {
    EXPECT_THROW(parse(R"(syntax = "proto4";)"), ParseError);
}

TEST(ProtoParserTest, SyntaxWithoutSemicolonThrows) {
    EXPECT_THROW(parse(R"(syntax = "proto3" message A {})"), ParseError);
}

TEST(ProtoParserTest, SimplePackage) {
    auto proto = parse("package tests;");
    EXPECT_EQ(proto.package, "tests");
}

TEST(ProtoParserTest, DottedPackage) {
    auto proto = parse("package com.example.foo;");
    EXPECT_EQ(proto.package, "com.example.foo");
}

TEST(ProtoParserTest, PackageWithoutSemicolonThrows) {
    EXPECT_THROW(parse("package tests message A {}"), ParseError);
}

TEST(ProtoParserTest, Imports) {
    auto proto = parse(R"(
        syntax = "proto3";
        import "a.proto";
        import "dir/b.proto";
    )");
    ASSERT_EQ(proto.imports.size(), 2);
    EXPECT_EQ(proto.imports[0], "a.proto");
    EXPECT_EQ(proto.imports[1], "dir/b.proto");
}

TEST(ProtoParserTest, ImportWithoutPathThrows) {
    EXPECT_THROW(parse("import foo;"), ParseError);
}

// ---------------------------------------------------------------------------
// Messages and fields
// ---------------------------------------------------------------------------

TEST(ProtoParserTest, EmptyMessage) {
    auto proto = parse("message Empty {}");
    ASSERT_EQ(proto.messages.size(), 1);
    EXPECT_EQ(proto.messages[0]->name, "Empty");
    EXPECT_TRUE(proto.messages[0]->fields.empty());
    EXPECT_TRUE(proto.messages[0]->nested_messages.empty());
    EXPECT_TRUE(proto.messages[0]->nested_enums.empty());
}

TEST(ProtoParserTest, SingleField) {
    auto proto = parse("message Test { int32 id = 1; }");
    ASSERT_EQ(proto.messages.size(), 1);
    const auto& msg = *proto.messages[0];
    ASSERT_EQ(msg.fields.size(), 1);
    EXPECT_EQ(msg.fields[0].name, "id");
    EXPECT_EQ(msg.fields[0].type, "int32");
    EXPECT_EQ(msg.fields[0].number, 1);
    EXPECT_FALSE(msg.fields[0].repeated);
    EXPECT_FALSE(msg.fields[0].packed);
}

TEST(ProtoParserTest, AllScalarTypes) {
    auto proto = parse(R"(
        message AllTypes {
            double   a_double   = 1;
            float    a_float    = 2;
            int32    a_int32    = 3;
            int64    a_int64    = 4;
            uint32   a_uint32   = 5;
            uint64   a_uint64   = 6;
            sint32   a_sint32   = 7;
            sint64   a_sint64   = 8;
            fixed32  a_fixed32  = 9;
            fixed64  a_fixed64  = 10;
            sfixed32 a_sfixed32 = 11;
            sfixed64 a_sfixed64 = 12;
            bool     a_bool     = 13;
            string   a_string   = 14;
            bytes    a_bytes    = 15;
        }
    )");
    ASSERT_EQ(proto.messages.size(), 1);
    const auto& fields = proto.messages[0]->fields;
    const std::vector<std::string> types = {
        "double", "float", "int32", "int64", "uint32", "uint64", "sint32", "sint64",
        "fixed32", "fixed64", "sfixed32", "sfixed64", "bool", "string", "bytes"};
    ASSERT_EQ(fields.size(), types.size());
    for (std::size_t i = 0; i < types.size(); ++i) {
        EXPECT_EQ(fields[i].type, types[i]);
        EXPECT_EQ(fields[i].name, "a_" + types[i]);
        EXPECT_EQ(fields[i].number, static_cast<int>(i + 1));
        EXPECT_FALSE(fields[i].repeated);
    }
}

TEST(ProtoParserTest, LargeFieldNumber) {
    auto proto = parse("message M { int32 x = 536870911; }");
    ASSERT_EQ(proto.messages.size(), 1);
    EXPECT_EQ(proto.messages[0]->fields[0].number, 536870911);
}

TEST(ProtoParserTest, MultipleMessagesKeepOrder) {
    auto proto = parse(R"(
        message First  { int32 a = 1; }
        message Second { int32 b = 1; }
        message Third  { int32 c = 1; }
    )");
    ASSERT_EQ(proto.messages.size(), 3);
    EXPECT_EQ(proto.messages[0]->name, "First");
    EXPECT_EQ(proto.messages[1]->name, "Second");
    EXPECT_EQ(proto.messages[2]->name, "Third");
}

TEST(ProtoParserTest, MessageTypedField) {
    auto proto = parse(R"(
        message Nested { sint32 value = 1; }
        message Outer  { Nested nested = 1; }
    )");
    ASSERT_EQ(proto.messages.size(), 2);
    const auto& field = proto.messages[1]->fields.at(0);
    EXPECT_EQ(field.type, "Nested");
    EXPECT_EQ(field.name, "nested");
    EXPECT_EQ(field.number, 1);
}

TEST(ProtoParserTest, QualifiedTypeName) {
    auto proto = parse("message M { other.pkg.Type value = 1; }");
    ASSERT_EQ(proto.messages.size(), 1);
    ASSERT_EQ(proto.messages[0]->fields.size(), 1);
    EXPECT_EQ(proto.messages[0]->fields[0].type, "other.pkg.Type");
    EXPECT_EQ(proto.messages[0]->fields[0].name, "value");
}

TEST(ProtoParserTest, OptionalAndRequiredModifiers) {
    auto proto = parse(R"(
        syntax = "proto2";
        message M {
            optional int32 a = 1;
            required string b = 2;
        }
    )");
    ASSERT_EQ(proto.messages.size(), 1);
    const auto& msg = *proto.messages[0];
    ASSERT_EQ(msg.fields.size(), 2);
    EXPECT_EQ(msg.fields[0].type, "int32");
    EXPECT_EQ(msg.fields[0].name, "a");
    EXPECT_FALSE(msg.fields[0].repeated);
    EXPECT_EQ(msg.fields[1].type, "string");
    EXPECT_EQ(msg.fields[1].name, "b");
    EXPECT_FALSE(msg.fields[1].repeated);
}

TEST(ProtoParserTest, NestedMessage) {
    auto proto = parse(R"(
        message Outer {
            message Inner { int32 x = 1; }
            Inner inner = 1;
        }
    )");
    ASSERT_EQ(proto.messages.size(), 1);
    const auto& outer = *proto.messages[0];
    ASSERT_EQ(outer.nested_messages.size(), 1);
    EXPECT_EQ(outer.nested_messages[0]->name, "Inner");
    ASSERT_EQ(outer.nested_messages[0]->fields.size(), 1);
    EXPECT_EQ(outer.nested_messages[0]->fields[0].name, "x");
    ASSERT_EQ(outer.fields.size(), 1);
    EXPECT_EQ(outer.fields[0].type, "Inner");
}

TEST(ProtoParserTest, NestedEnum) {
    auto proto = parse(R"(
        message Outer {
            enum Kind { KIND_UNSPECIFIED = 0; KIND_A = 1; }
            Kind kind = 1;
        }
    )");
    ASSERT_EQ(proto.messages.size(), 1);
    const auto& outer = *proto.messages[0];
    ASSERT_EQ(outer.nested_enums.size(), 1);
    EXPECT_EQ(outer.nested_enums[0].name, "Kind");
    ASSERT_EQ(outer.nested_enums[0].values.size(), 2);
    ASSERT_EQ(outer.fields.size(), 1);
    EXPECT_EQ(outer.fields[0].type, "Kind");
}

// ---------------------------------------------------------------------------
// Repeated fields and packing
// ---------------------------------------------------------------------------

TEST(ProtoParserTest, RepeatedIntegerIsPackedByDefault) {
    auto proto = parse("message M { repeated sint32 values = 1; }");
    const auto& field = proto.messages.at(0)->fields.at(0);
    EXPECT_TRUE(field.repeated);
    EXPECT_TRUE(field.packed);
    EXPECT_EQ(field.type, "sint32");
}

TEST(ProtoParserTest, RepeatedStringIsNotPacked) {
    auto proto = parse("message M { repeated string names = 1; }");
    const auto& field = proto.messages.at(0)->fields.at(0);
    EXPECT_TRUE(field.repeated);
    EXPECT_FALSE(field.packed);
}

TEST(ProtoParserTest, RepeatedBytesIsNotPacked) {
    auto proto = parse("message M { repeated bytes blobs = 1; }");
    const auto& field = proto.messages.at(0)->fields.at(0);
    EXPECT_TRUE(field.repeated);
    EXPECT_FALSE(field.packed);
}

TEST(ProtoParserTest, RepeatedMessageIsNotPacked) {
    auto proto = parse(R"(
        message Item { int32 id = 1; }
        message M { repeated Item items = 1; }
    )");
    const auto& field = proto.messages.at(1)->fields.at(0);
    EXPECT_TRUE(field.repeated);
    EXPECT_FALSE(field.packed);
    EXPECT_EQ(field.type, "Item");
}

// In proto3 every repeated scalar numeric field is packed by default,
// including float, double and bool.
TEST(ProtoParserTest, RepeatedFloatingPointAndBoolArePackedByDefault) {
    auto proto = parse(R"(
        message M {
            repeated float  f = 1;
            repeated double d = 2;
            repeated bool   b = 3;
        }
    )");
    const auto& msg = *proto.messages.at(0);
    EXPECT_TRUE(field_named(msg, "f").packed);
    EXPECT_TRUE(field_named(msg, "d").packed);
    EXPECT_TRUE(field_named(msg, "b").packed);
}

TEST(ProtoParserTest, PackedFalseOption) {
    auto proto = parse("message M { repeated sint32 values = 1 [packed = false]; }");
    const auto& field = proto.messages.at(0)->fields.at(0);
    EXPECT_TRUE(field.repeated);
    EXPECT_FALSE(field.packed);
}

TEST(ProtoParserTest, PackedTrueOption) {
    auto proto = parse("message M { repeated sint32 values = 1 [packed = true]; }");
    const auto& field = proto.messages.at(0)->fields.at(0);
    EXPECT_TRUE(field.repeated);
    EXPECT_TRUE(field.packed);
}

TEST(ProtoParserTest, PackedOptionWithoutSpaces) {
    auto proto = parse("message M { repeated sint32 values = 1[packed=false]; }");
    const auto& field = proto.messages.at(0)->fields.at(0);
    EXPECT_FALSE(field.packed);
}

TEST(ProtoParserTest, UnknownFieldOptionIsIgnored) {
    auto proto = parse("message M { int32 old = 1 [deprecated = true]; int32 next = 2; }");
    const auto& msg = *proto.messages.at(0);
    ASSERT_EQ(msg.fields.size(), 2);
    EXPECT_EQ(msg.fields[0].name, "old");
    EXPECT_FALSE(msg.fields[0].packed);
    EXPECT_EQ(msg.fields[1].name, "next");
}

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

TEST(ProtoParserTest, TopLevelEnum) {
    auto proto = parse(R"(
        enum Color {
            COLOR_UNSPECIFIED = 0;
            COLOR_RED   = 1;
            COLOR_GREEN = 2;
        }
    )");
    ASSERT_EQ(proto.enums.size(), 1);
    const auto& e = proto.enums[0];
    EXPECT_EQ(e.name, "Color");
    ASSERT_EQ(e.values.size(), 3);
    EXPECT_EQ(e.values[0].name, "COLOR_UNSPECIFIED");
    EXPECT_EQ(e.values[0].number, 0);
    EXPECT_EQ(e.values[1].name, "COLOR_RED");
    EXPECT_EQ(e.values[1].number, 1);
    EXPECT_EQ(e.values[2].name, "COLOR_GREEN");
    EXPECT_EQ(e.values[2].number, 2);
}

TEST(ProtoParserTest, EnumWithNegativeValue) {
    auto proto = parse("enum E { E_ZERO = 0; E_NEG = -1; }");
    ASSERT_EQ(proto.enums.size(), 1);
    ASSERT_EQ(proto.enums[0].values.size(), 2);
    EXPECT_EQ(proto.enums[0].values[1].name, "E_NEG");
    EXPECT_EQ(proto.enums[0].values[1].number, -1);
}

TEST(ProtoParserTest, EnumTypedField) {
    auto proto = parse(R"(
        enum Month { MONTH_UNSPECIFIED = 0; MONTH_JANUARY = 1; }
        message Date { Month month = 2; }
    )");
    ASSERT_EQ(proto.enums.size(), 1);
    ASSERT_EQ(proto.messages.size(), 1);
    const auto& field = proto.messages[0]->fields.at(0);
    EXPECT_EQ(field.type, "Month");
    EXPECT_EQ(field.number, 2);
}

TEST(ProtoParserTest, EnumWithoutNameThrows) {
    EXPECT_THROW(parse("enum { A = 0; }"), ParseError);
}

TEST(ProtoParserTest, EnumValueWithoutSemicolonThrows) {
    EXPECT_THROW(parse("enum E { A = 0 B = 1; }"), ParseError);
}

// ---------------------------------------------------------------------------
// Comments
// ---------------------------------------------------------------------------

TEST(ProtoParserTest, CommentsAreIgnored) {
    auto proto = parse(R"(
        // leading line comment
        syntax = "proto3"; // trailing comment
        /* block
           comment */
        package tests;
        message M {
            int32 a = 1; // comment after field
            /* comment between fields */
            int32 b = 2;
        }
    )");
    EXPECT_EQ(proto.syntax, "proto3");
    EXPECT_EQ(proto.package, "tests");
    ASSERT_EQ(proto.messages.size(), 1);
    ASSERT_EQ(proto.messages[0]->fields.size(), 2);
    EXPECT_EQ(proto.messages[0]->fields[0].name, "a");
    EXPECT_EQ(proto.messages[0]->fields[1].name, "b");
}

// ---------------------------------------------------------------------------
// Errors in message bodies
// ---------------------------------------------------------------------------

TEST(ProtoParserTest, MessageWithoutNameThrows) {
    EXPECT_THROW(parse("message { int32 a = 1; }"), ParseError);
}

TEST(ProtoParserTest, MessageWithoutOpeningBraceThrows) {
    EXPECT_THROW(parse("message M int32 a = 1; }"), ParseError);
}

TEST(ProtoParserTest, UnterminatedMessageThrows) {
    EXPECT_THROW(parse("message M { int32 a = 1;"), ParseError);
}

TEST(ProtoParserTest, FieldWithoutEqualsThrows) {
    EXPECT_THROW(parse("message M { int32 a 1; }"), ParseError);
}

TEST(ProtoParserTest, FieldWithoutNumberThrows) {
    EXPECT_THROW(parse("message M { int32 a = ; }"), ParseError);
}

TEST(ProtoParserTest, FieldWithoutSemicolonThrows) {
    EXPECT_THROW(parse("message M { int32 a = 1 int32 b = 2; }"), ParseError);
}

TEST(ProtoParserTest, UnterminatedOptionListThrows) {
    EXPECT_THROW(parse("message M { repeated int32 a = 1 [packed = true; }"), ParseError);
}

// ---------------------------------------------------------------------------
// A complete file, as used by the datetime example
// ---------------------------------------------------------------------------

TEST(ProtoParserTest, CompleteDatetimeFile) {
    auto proto = parse(R"(
        syntax = "proto3";
        package datetime;

        enum Month {
            MONTH_UNSPECIFIED = 0;
            MONTH_JANUARY     = 1;
            MONTH_FEBRUARY    = 2;
            MONTH_MARCH       = 3;
            MONTH_APRIL       = 4;
            MONTH_MAY         = 5;
            MONTH_JUNE        = 6;
            MONTH_JULY        = 7;
            MONTH_AUGUST      = 8;
            MONTH_SEPTEMBER   = 9;
            MONTH_OCTOBER     = 10;
            MONTH_NOVEMBER    = 11;
            MONTH_DECEMBER    = 12;
        }

        message Date {
            uint32 day   = 1;
            Month month = 2;
            int32 year  = 3;
        }

        message DateTime {
            Date date = 1;
            sint32 seconds_from_noon = 2;
        }
    )");

    EXPECT_EQ(proto.syntax, "proto3");
    EXPECT_EQ(proto.package, "datetime");

    ASSERT_EQ(proto.enums.size(), 1);
    EXPECT_EQ(proto.enums[0].name, "Month");
    ASSERT_EQ(proto.enums[0].values.size(), 13);
    EXPECT_EQ(proto.enums[0].values[12].name, "MONTH_DECEMBER");
    EXPECT_EQ(proto.enums[0].values[12].number, 12);

    ASSERT_EQ(proto.messages.size(), 2);
    const auto& date = *proto.messages[0];
    EXPECT_EQ(date.name, "Date");
    ASSERT_EQ(date.fields.size(), 3);
    EXPECT_EQ(field_named(date, "day").type, "uint32");
    EXPECT_EQ(field_named(date, "month").type, "Month");
    EXPECT_EQ(field_named(date, "year").type, "int32");
    EXPECT_EQ(field_named(date, "year").number, 3);

    const auto& datetime = *proto.messages[1];
    EXPECT_EQ(datetime.name, "DateTime");
    ASSERT_EQ(datetime.fields.size(), 2);
    EXPECT_EQ(field_named(datetime, "date").type, "Date");
    EXPECT_EQ(field_named(datetime, "seconds_from_noon").type, "sint32");
    EXPECT_EQ(field_named(datetime, "seconds_from_noon").number, 2);
}
