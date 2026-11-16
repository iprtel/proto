#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iprtl/proto/encode.h"
#include <limits>


using namespace iprtl::proto;

TEST(EncodeTest, encode_fixed) {
    std::array<uint8_t, 128> buffer{};
    auto remaining = encode<float_p>(buffer, 12.375f);
    EXPECT_EQ(remaining.size(), buffer.size() - 4);
    EXPECT_EQ(buffer[0], 0x00);
    EXPECT_EQ(buffer[1], 0x00);
    EXPECT_EQ(buffer[2], 0x46);
    EXPECT_EQ(buffer[3], 0x41);
}



TEST(EncodeTest, encode_varint_sint32) {
    std::array<uint8_t, 128> buffer{};
    auto remaining = encode<sint32_p>(buffer, -1);
    EXPECT_EQ(remaining.size(), buffer.size() - 1);
    EXPECT_EQ(buffer[0], 0x01);
    remaining = encode<sint32_p>(buffer, 0);
    EXPECT_EQ(remaining.size(), buffer.size() - 1);
    EXPECT_EQ(buffer[0], 0x00);
    remaining = encode<sint32_p>(buffer, 1);
    EXPECT_EQ(remaining.size(), buffer.size() - 1);
    EXPECT_EQ(buffer[0], 0x02);
}

TEST(EncodeTest, encode_sint32_field) {
    // proto:
    // message Test{
    //   sint32 test = 1;
    // }
    // {"test":0} -> 0800 = 2
    // {"test":1} -> 0802 = 2
    // {"test":-1} -> 0801 = 2
    struct Test {
        sint32_p::cpp_type test;
    };
    using Field = meta::ProtoField<struct Test, 1, sint32_p, "test", &Test::test>;
    std::array<uint8_t, 128> buffer{};
    auto remaining = encode<Field>(buffer, Test{1});
    EXPECT_EQ(remaining.size(), buffer.size() - 2);
    EXPECT_EQ(buffer[0], 0x08);
    EXPECT_EQ(buffer[1], 0x02);
}

TEST(EncodeTest, encode_packedsint32_field) {
    // proto:
    // message Test{
    //   repeated sint32 test = 1;
    // }
    // {"test":[-1,0,1]} -> 0a03010002 = 5
    // {"test":[-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]} -> 0a120100020406080a0c0e10121416181a1c1e20 = 20
    struct Test {
        std::vector<sint32_p::cpp_type> test;
    };
    using Field = meta::ProtoField<struct Test, 1, packed_p<sint32_p>, "test", &Test::test>;
    std::array<uint8_t, 128> buffer{};
    auto remaining = encode<Field>(buffer, Test{{-1,0,1}});
    EXPECT_EQ(remaining.size(), buffer.size() - 5);
    EXPECT_EQ(buffer[0], 0x0a);
    EXPECT_EQ(buffer[1], 0x03);
    EXPECT_EQ(buffer[2], 0x01);
    EXPECT_EQ(buffer[3], 0x00);
    EXPECT_EQ(buffer[4], 0x02);
}


TEST(EncodeTest, length_unpackedsint32_field) {
   // message Test{
   //   repeated sint32 test = 1[packed=false];
   // }
   // {"test":[-1,0,1]} -> 0801 0800 0802 = 6
   // {"test":[-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]} -> 0801 0800 0802 0804 0806 0808 080a 080c 080e 0810 0812 0814 0816 0818 081a 081c 081e 0820 = 36
    struct Test {
        std::vector<sint32_p::cpp_type> test;
    };
    using Field = meta::ProtoField<struct Test, 1, repeated_p<sint32_p>, "test", &Test::test>;
    std::array<uint8_t, 128> buffer{};
    auto remaining = encode<Field>(buffer, Test{{-1,0,1}});
    EXPECT_EQ(remaining.size(), buffer.size() - 6);
}


struct Nested {
    sint32_p::cpp_type a;
};
bool operator==(const Nested& lhs, const Nested& rhs) {
    return lhs.a == rhs.a;
}
using Proto_Nested = iprtl::proto::meta::ProtoMessage<Nested, "Nested",
                     iprtl::proto::meta::ProtoField<Nested,1,iprtl::proto::sint32_p,"a",&Nested::a>
                     >;
struct TestNested {
    Nested test{};
};
TEST(EncodeTest, encode_nested_field) {
    // message Nested {
    //   sint32  a = 1;
    // }
    // message Test{
    //   Nested test = 1;
    // }
    // {"test":{"a":1}} -> 0a020802 = 4
    using Field = meta::ProtoField<struct TestNested, 1, nested_p<Proto_Nested>, "test", &TestNested::test>;

    std::array<uint8_t, 128> buffer{};
    auto remaining = encode<Proto_Nested>(buffer, Nested{1});
    EXPECT_EQ(remaining.size(), buffer.size() - 2);
    EXPECT_EQ(buffer[0], 0x08);
    EXPECT_EQ(buffer[1], 0x02);

    remaining = encode<Field>(buffer, TestNested{Nested{1}});
    EXPECT_EQ(remaining.size(), buffer.size() - 4);
    EXPECT_EQ(buffer[0], 0x0a);
    EXPECT_EQ(buffer[1], 0x02);
    EXPECT_EQ(buffer[2], 0x08);
    EXPECT_EQ(buffer[3], 0x02);
}
