#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include "iprtl/enums.h"
#include "iprtl/template_string.h"
#include "fmt/core.h"
#include "fmt/ranges.h"

namespace iprtl::proto::wire {

/*
0	VARINT	int32, int64, uint32, uint64, sint32, sint64, bool, enum
1	I64	fixed64, sfixed64, double
2	LEN	string, bytes, embedded messages, packed repeated fields
3	SGROUP	group start (deprecated)
4	EGROUP	group end (deprecated)
5	I32	fixed32, sfixed32, float
*/
ENUMS(type_id, uint8_t,
    VARINT = 0,
    I64    = 1,
    LEN    = 2,
    SGROUP = 3,
    EGROUP = 4,
    I32    = 5
)

//enum class varint : uint64_t {};
//enum class i32 : uint32_t {};
//enum class i64 : uint64_t {};
//using len_data = std::vector<uint8_t>;

template<typename cpp_t, type_id id,TemplateString name_literal>
struct wire_type {
    static constexpr type_id type = id;
    using cpp_type = cpp_t;
    cpp_t content;
    static constexpr std::string name {name_literal.value,name_literal.size};
    operator cpp_t () const {return content;}
    constexpr bool operator==(const wire_type<cpp_t, id, name_literal>& other) const {return this->content == other.content;};
};
using varint = wire_type<uint64_t, type_id::VARINT, "varint">;
using i32 = wire_type<uint32_t, type_id::I32, "i32">;
using i64 = wire_type<uint64_t, type_id::I64, "i64">;
using len_data = wire_type<std::vector<uint8_t>, type_id::LEN, "len_data">;

template<typename cpp_t, type_id id,TemplateString name_literal>
auto format_as(wire_type<cpp_t, id, name_literal> const& v){
    return fmt::format("{}",v.content);
}
template<typename wire_type>
varint tag(uint32_t field_number) {
    return varint{(field_number << 3) | static_cast<uint8_t>(wire_type::type)};
}

using wire_value=std::variant<varint, i32, i64, len_data>;
auto format_as(wire_value const& v){
    auto result = std::visit([](auto v){
        return format_as(v);
    }, v);
    return result;
}


struct record {
    uint32_t field_number;
    wire_value value;
};
auto format_as(record const& v){
    auto result = fmt::format("{}: {}", v.field_number, v.value);
    return result;
}

}