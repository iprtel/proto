#pragma once

#include <string>
#include <vector>
#include "wire/types.h"

namespace iprtl::proto {
/*
double	Uses IEEE 754 double-precision format.
float	Uses IEEE 754 single-precision format.
int32	Uses variable-length encoding. Inefficient for encoding negative numbers – if your field is likely to have negative values, use sint32 instead.
int64	Uses variable-length encoding. Inefficient for encoding negative numbers – if your field is likely to have negative values, use sint64 instead.
uint32	Uses variable-length encoding.
uint64	Uses variable-length encoding.
sint32	Uses variable-length encoding. Signed int value. These more efficiently encode negative numbers than regular int32s.
sint64	Uses variable-length encoding. Signed int value. These more efficiently encode negative numbers than regular int64s.
fixed32	Always four bytes. More efficient than uint32 if values are often greater than 228.
fixed64	Always eight bytes. More efficient than uint64 if values are often greater than 256.
sfixed32	Always four bytes.
sfixed64	Always eight bytes.
bool	Uses variable-length encoding.
string	A string must always contain UTF-8 encoded or 7-bit ASCII text, and cannot be longer than 232.
bytes	May contain any arbitrary sequence of bytes no longer than 232.
*/
ENUMS(field_type, uint8_t,
    Double, Float, Int32, Int64, Uint32, Uint64, Sint32, Sint64, Fixed32, Fixed64,
    Sfixed32, Sfixed64, Bool, String, Bytes, Group, Message, Enum)



ENUMS(field_cardinality, uint8_t,
    Implicit, Optional, Required, Repeated)


struct int32_p {
    using cpp_type = int32_t;
    using wire_type = wire::varint;
};
struct int64_p {
    using cpp_type = int64_t;
    using wire_type = wire::varint;
};
struct uint32_p {
    using cpp_type = uint32_t;
    using wire_type = wire::varint;
};
struct uint64_p {
    using cpp_type = uint64_t;
    using wire_type = wire::varint;
};
struct sint32_p {
    using cpp_type = int32_t;
    using wire_type = wire::varint;
};
struct sint64_p {
    using cpp_type = int64_t;
    using wire_type = wire::varint;
};
struct fixed32_p {
    using cpp_type = uint32_t;
    using wire_type = wire::i32;
};
struct fixed64_p {
    using cpp_type = uint64_t;
    using wire_type = wire::i64;
};
struct sfixed32_p {
    using cpp_type = int32_t;
    using wire_type = wire::i32;
};
struct sfixed64_p {
    using cpp_type = int64_t;
    using wire_type = wire::i64;
};
struct bool_p {
    using cpp_type = bool;
    using wire_type = wire::varint;
};
struct double_p {
    using cpp_type = double;
    using wire_type = wire::i64;
};
struct float_p {
    using cpp_type = float;
    using wire_type = wire::i32;
};
struct string_p {
    using cpp_type = std::string;
    using wire_type = wire::len_data;
};
struct bytes_p {
    using cpp_type = std::vector<uint8_t>;
    using wire_type = wire::len_data;
};
template<typename T>
struct enum_p {
    using cpp_type = T;
    using wire_type = wire::varint;
};

template<typename ElementType>
struct repeated_p {
    using element_type = ElementType;
    using cpp_type = std::vector<typename ElementType::cpp_type>;
    using wire_type = ElementType::wire_type;
};

template<typename ElementType>
struct packed_p {
    using element_type = ElementType;
    using cpp_type = std::vector<typename ElementType::cpp_type>;
    using wire_type = wire::len_data;
};


}