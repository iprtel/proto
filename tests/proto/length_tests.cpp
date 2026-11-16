#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iprtl/proto/meta/length.h"
#include "iprtl/proto/meta/operators.h"
#include <limits>


using namespace iprtl::proto;
using namespace iprtl::proto::meta;

TEST(LengthTest, length_fixed) {
    EXPECT_EQ(length<fixed32_p>(0u),4);
    EXPECT_EQ(length<float_p>(float{}),4);
    EXPECT_EQ(length<fixed64_p>(uint64_t{}),8);
    EXPECT_EQ(length<double_p>(double{}),8);
}

TEST(LengthTest, length_varint_sint32) {
    EXPECT_EQ(length<sint32_p>(0),1);
    EXPECT_EQ(length<sint32_p>(1),1);
    EXPECT_EQ(length<sint32_p>(2),1);
    EXPECT_EQ(length<sint32_p>(127),2);
    EXPECT_EQ(length<sint32_p>(128),2);
    EXPECT_EQ(length<sint32_p>(std::numeric_limits<typename sint32_p::cpp_type>::max()),5);
    EXPECT_EQ(length<sint32_p>(-1),1);
    EXPECT_EQ(length<sint32_p>(std::numeric_limits<typename sint32_p::cpp_type>::min()),5);
}

TEST(LengthTest, length_varint_int32) {
    EXPECT_EQ(length<int32_p>(0),1);
    EXPECT_EQ(length<int32_p>(1),1);
    EXPECT_EQ(length<int32_p>(2),1);
    EXPECT_EQ(length<int32_p>(127),1);
    EXPECT_EQ(length<int32_p>(128),2);
    EXPECT_EQ(length<int32_p>(std::numeric_limits<typename sint32_p::cpp_type>::max()),5);
    EXPECT_EQ(length<int32_p>(-1),10);
    EXPECT_EQ(length<int32_p>(std::numeric_limits<typename sint32_p::cpp_type>::min()),10);
}

TEST(LengthTest, length_string) {
    EXPECT_EQ(length<string_p>(""),1);
    EXPECT_EQ(length<string_p>("hello"),6);
}

//TEST(LengthTest, length_repeated_string) {
//    EXPECT_EQ(length<repeated_p<string_p>>({""}),1);
//    EXPECT_EQ(length<repeated_p<string_p>>({"",""}),2);
//    EXPECT_EQ(length<repeated_p<string_p>>({"hello"}),6);
//}

TEST(LengthTest, length_sint32_field) {
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
    EXPECT_EQ(length<Field>(Test{}),0);
    EXPECT_EQ(length<Field>(Test{0}),0);
    EXPECT_EQ(length<Field>(Test{1}),2);
    EXPECT_EQ(length<Field>(Test{127}),3);
    EXPECT_EQ(length<Field>(Test{128}),3);
}

TEST(LengthTest, length_packedsint32_field) {
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
    EXPECT_EQ(length<Field>(Test{{-1,0,1}}),5);
    EXPECT_EQ(length<Field>(Test{{-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}}),20);
}


TEST(LengthTest, length_unpackedsint32_field) {
   // message Test{
   //   repeated sint32 test = 1[packed=false];
   // }
   // {"test":[-1,0,1]} -> 0801 0800 0802 = 6
   // {"test":[-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]} -> 0801 0800 0802 0804 0806 0808 080a 080c 080e 0810 0812 0814 0816 0818 081a 081c 081e 0820 = 36
    struct Test {
        std::vector<sint32_p::cpp_type> test;
    };
    using Field = meta::ProtoField<struct Test, 1, repeated_p<sint32_p>, "test", &Test::test>;
    EXPECT_EQ(length<Field>(Test{{-1,0,1}}),6);
    EXPECT_EQ(length<Field>(Test{{-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}}),36);
}


struct Nested {
    int32_t value{};
};
using Proto_Nested = iprtl::proto::meta::ProtoMessage<Nested, "Nested",
    iprtl::proto::meta::ProtoField<Nested,1,iprtl::proto::sint32_p,"value",&Nested::value>>;
inline bool operator==(const Nested& lhs, const Nested& rhs) { return iprtl::proto::meta::equal<Proto_Nested>(lhs, rhs); }

struct NestedTest {
    Nested nested{};
};
using Proto_NestedTest = iprtl::proto::meta::ProtoMessage<NestedTest, "NestedTest",
    iprtl::proto::meta::ProtoField<NestedTest,1,iprtl::proto::meta::nested_p<Proto_Nested>,"nested",&NestedTest::nested>>;
inline bool operator==(const NestedTest& lhs, const NestedTest& rhs) { return iprtl::proto::meta::equal<Proto_NestedTest>(lhs, rhs); }

TEST(LengthTest, length_nested_field) {
    // message Nested {
    //   sint32  a = 1;
    // }
    // message Test{
    //   Nested test = 1;
    // }
    // {"test":{"a":1}} -> 0a020802 = 4
    EXPECT_EQ(length<Proto_Nested>(Nested{1}),2);
    EXPECT_EQ(length<Proto_NestedTest>(NestedTest{Nested{1}}),4);
}