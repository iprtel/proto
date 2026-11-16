# proto — a Protocol Buffers implementation in modern C++

A small, header-only implementation of the [Protocol Buffers](https://protobuf.dev/) wire format in C++20, together with a code generator (`protopiler`) that turns `.proto` files into plain C++ structs and the compile-time metadata needed to encode, decode, compare and print them.

> **This is a learning project.** I wrote it to explore and practise:
>
> - **Typed programming:** using the type system to encode meaning, so that mistakes become compile errors instead of runtime bugs.
> - **ADL (argument-dependent lookup):** generated free functions that are found automatically through the namespace of the message type.
> - **Generic field access:** describing struct members at compile time (member pointers, type lists, `std::variant`) and reaching them from runtime data such as a field number.
> - **Varint and ZigZag encoding:** the variable-length integer encoding at the heart of the protobuf wire format.
> - Templates, concepts, non-type template parameters (compile-time strings), `std::span`, `std::visit`, and `{fmt}` integration.
>
> It is not meant to replace Google's `protobuf` library. It covers only part of the language and has not been hardened for production use.

---

## What it does

Given a `.proto` file such as

```proto
syntax = "proto3";
package datetime;

enum Month {
    MONTH_UNSPECIFIED = 0;
    MONTH_JANUARY     = 1;
    // ...
}

message Date {
    uint32 day   = 1;
    Month  month = 2;
    int32  year  = 3;
}

message DateTime {
    Date   date              = 1;
    sint32 seconds_from_noon = 2;
}
```

`protopiler` generates a header `datetime.proto.h` with plain aggregate structs:

```cpp
namespace datetime {
struct Date {
    uint32_t day{};
    Month    month{};
    int32_t  year{};
};
// ...
}
```

Each struct also gets a compile-time description of its fields and a set of free functions in the same namespace:

```cpp
using Proto_Date = iprtl::proto::meta::ProtoMessage<Date, "Date",
    iprtl::proto::meta::ProtoField<Date, 1, iprtl::proto::uint32_p,    "day",   &Date::day>,
    iprtl::proto::meta::ProtoField<Date, 2, iprtl::proto::enum_p<Month>, "month", &Date::month>,
    iprtl::proto::meta::ProtoField<Date, 3, iprtl::proto::int32_p,     "year",  &Date::year>>;

inline auto format_as(Date const& v);                                   // {fmt} support
inline std::vector<uint8_t> encode_message(Date const& v);              // serialize
inline auto encode_message(Date const& v, std::span<uint8_t> buffer);   // serialize into a buffer
inline auto decode_message(Date& v, std::span<const uint8_t> buffer);   // parse
inline bool operator==(const Date& lhs, const Date& rhs);               // field-wise compare
inline std::size_t encoded_length(const Date& msg);                     // size before encoding
```

Using the generated code feels like working with ordinary C++ structs:

```cpp
#include <fmt/core.h>
#include "datetime.proto.h"

datetime::DateTime dt = {
    .date{.day = 31, .month = datetime::Month::MONTH_DECEMBER, .year = 2025},
    .seconds_from_noon = -1,
};

fmt::print("{}\n", dt);                // human-readable output
auto bytes = encode_message(dt);       // std::vector<uint8_t>, protobuf wire format

datetime::DateTime decoded{};
decode_message(decoded, bytes);
assert(decoded == dt);
```

The bytes are compatible with the official protobuf wire format.

### Supported features

- Scalar types: `int32`, `int64`, `uint32`, `uint64`, `sint32`, `sint64`, `fixed32`, `fixed64`, `sfixed32`, `sfixed64`, `bool`, `float`, `double`
- `string` and `bytes`
- Enums (top-level and nested)
- Nested messages
- `repeated` fields, including packed encoding (the proto3 default for scalars, or set with `[packed = true]`)
- Encoding, decoding, length calculation, equality and `{fmt}` formatting

Not supported (yet): `oneof`, `map`, services, extensions, groups, and full handling of `import`.

---

## The ideas behind it

### Typed programming

The library uses distinct types where a plain integer would also "work":

- **Wire types** (`varint`, `i32`, `i64`, `len_data` in [wire/types.h](src/iprtl/proto/wire/types.h)) are separate types. Each knows its own wire type id, and overloads choose the correct encoding for each one.
- **Proto types** (`int32_p`, `sint32_p`, `fixed32_p`, … in [types.h](src/iprtl/proto/types.h)) are tag types. Each pairs a C++ type (`cpp_type`) with a wire type (`wire_type`). `int32`, `sint32` and `sfixed32` all map to `int32_t` in C++, but they are encoded very differently. The tag type picks the right encoding at compile time.
- **Concepts** ([concepts.h](src/iprtl/proto/concepts.h)) restrict templates to valid message and field descriptions.
- **Compile-time strings** (`FixedString`, `TemplateString`) let names such as `"day"` be template arguments, so field names are available for formatting without any runtime registry.

### ADL — argument-dependent lookup

The generator puts `encode_message`, `decode_message`, `format_as`, `operator==` and `encoded_length` into the **same namespace as the generated struct**. Because of ADL, user code can call `encode_message(dt)` without qualifying it, and `{fmt}` finds `format_as` automatically, so `fmt::print("{}", dt)` works with no extra registration.

### Generic field access

A message is described by a `ProtoMessage<Class, Name, ProtoField...>` type list ([meta/types.h](src/iprtl/proto/meta/types.h)). Each `ProtoField` stores the field number, proto type, name and a **pointer to member** (`&Date::day`). Generic algorithms can therefore walk over all fields at compile time to encode, compare, format or compute lengths.

Decoding goes in the other direction: a field number read from the wire at runtime has to be mapped to a compile-time field description. [support.h](src/iprtl/proto/support.h) builds a `std::variant` of all field accessors and selects the right one by field number. `std::visit` then dispatches to the correctly typed `set_field` ([meta/decode.h](src/iprtl/proto/meta/decode.h)).

### Varint and ZigZag

Protobuf stores most integers as **varints**: 7 bits of payload per byte, with the high bit marking that another byte follows. Small numbers need fewer bytes. Negative `int32`/`int64` values always take 10 bytes. For that reason, `sint32`/`sint64` first apply **ZigZag** encoding (`(n << 1) ^ (n >> 63)`), which maps small negative numbers to small unsigned numbers. See [wire/read.h](src/iprtl/proto/wire/read.h), [wire/encode.h](src/iprtl/proto/wire/encode.h) and [as.h](src/iprtl/proto/as.h). The tests in [tests/wire/varint_tests.cpp](tests/wire/varint_tests.cpp) and [tests/proto/message/basic_ints_tests.cpp](tests/proto/message/basic_ints_tests.cpp) show the exact byte sequences.

---

## Project layout

```
src/iprtl/
├── enums.h, template_string.h   Helpers: reflective enums, compile-time strings
└── proto/
    ├── wire/                    Low-level wire format: varint, i32, i64, len_data, records
    ├── meta/                    Compile-time message/field descriptions, decoding, formatting
    ├── types.h, encode.h, …     Proto type tags and the generic encode/length/compare layer
    ├── meta.h                   Main entry point included by generated headers
    ├── compiler/                protopiler: tokenizer, parser, AST and C++ code generator
    └── tools/                   wire-dump: prints the raw records of a binary message
src/examples/datetime/           Example program using generated code
tests/                           GoogleTest suites for wire layer, tokenizer and messages
```

---

## Building

Requirements: a C++20 compiler (recent GCC or Clang) and CMake ≥ 3.28. The dependencies ([{fmt}](https://github.com/fmtlib/fmt), [GoogleTest](https://github.com/google/googletest), [argparse](https://github.com/p-ranav/argparse)) are downloaded automatically with CMake `FetchContent`.

```bash
cmake -B build
cmake --build build
ctest --test-dir build
```

The build first compiles `protopiler`, then uses it to generate headers for the tests and examples.

### Tools

- **protopiler** generates C++ headers from `.proto` files:

  ```bash
  build/bin/protopiler --out-dir <dir> [--base_ns <namespace>] file.proto ...
  ```

- **wire-dump** reads a binary protobuf message from stdin and prints its records (field number, wire type, raw value):

  ```bash
  build/src/iprtl/proto/tools/wire-dump < message.bin
  ```

- **datetime-ex** is a small example that builds, prints and encodes a `DateTime` message. It shows that a `uint32`/`int32`/`sint32` schema and a fixed-width schema hold the same data but produce different bytes.

### Using it in your own CMake project

The library is header-only. Add `src/` to your include path, link `{fmt}`, and run `protopiler` on your `.proto` files. See the `prepare_proto` macro in [src/examples/CMakeLists.txt](src/examples/CMakeLists.txt) for an example.
