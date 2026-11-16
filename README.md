# proto — a Protocol Buffers implementation in modern C++

A small, header-only implementation of the [Protocol Buffers](https://protobuf.dev/) wire format in C++20, together with a code generator (`protopiler`) that turns `.proto` files into plain C++ structs and the compile-time metadata needed to encode, decode, compare and print them.

> **This is a learning project.** I wrote it to explore and practise:
>
> - **Typed programming:** using the type system to encode meaning, so that mistakes become compile errors instead of runtime bugs.
> - **ADL (argument-dependent lookup):** generated free functions that are found automatically through the namespace of the message type.
> - **Generic field access:** describing struct members at compile time (member pointers, type lists, fold expressions) and reaching them from runtime data such as a field number.
> - **Varint and ZigZag encoding:** the variable-length integer encoding at the heart of the protobuf wire format.
> - **Layered design with customization points:** one `codec<T>` class template that lower layers declare and higher layers specialize, so dependencies only point downwards.
> - Templates, concepts, constrained partial specializations, non-type template parameters (compile-time strings), `std::span`, and `{fmt}` integration.
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

A field that holds another message uses `nested_p`:

```cpp
using Proto_DateTime = iprtl::proto::meta::ProtoMessage<DateTime, "DateTime",
    iprtl::proto::meta::ProtoField<DateTime, 1, iprtl::proto::meta::nested_p<Proto_Date>, "date", &DateTime::date>,
    iprtl::proto::meta::ProtoField<DateTime, 2, iprtl::proto::sint32_p, "seconds_from_noon", &DateTime::seconds_from_noon>>;
```

The generated functions are thin wrappers around the templates in [meta.h](src/iprtl/proto/meta.h) (`iprtl::proto::encode_message<Proto_Date>(v)` and so on).

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
- **Concepts** restrict templates to the types they can handle: [concepts.h](src/iprtl/proto/concepts.h) groups the proto types (varint, fixed, length-delimited, packed, …) and [meta/concepts.h](src/iprtl/proto/meta/concepts.h) recognizes message and field descriptions.
- **Compile-time strings** (`FixedString`, `TemplateString`) let names such as `"day"` be template arguments, so field names are available for formatting without any runtime registry.

### ADL — argument-dependent lookup

The generator puts `encode_message`, `decode_message`, `format_as`, `operator==` and `encoded_length` into the **same namespace as the generated struct**. Because of ADL, user code can call `encode_message(dt)` without qualifying it, and `{fmt}` finds `format_as` automatically, so `fmt::print("{}", dt)` works with no extra registration.

### Generic field access

A message is described by a `ProtoMessage<Class, Name, ProtoField...>` type list ([meta/types.h](src/iprtl/proto/meta/types.h)). Each `ProtoField` stores the field number, proto type, name and a **pointer to member** (`&Date::day`). Generic algorithms can therefore walk over all fields at compile time to encode, compare, format or compute lengths.

Because the fields are a template parameter pack, these algorithms are fold expressions. Computing the length of a message is simply the sum over its fields:

```cpp
static std::size_t length(value_type const& message) {
    return (std::size_t{0} + ... + codec<Fields>::length(message));
}
```

Formatting and comparison use `ProtoMessage::for_each_field`, which calls a templated lambda once per field.

Decoding goes in the other direction: a field number read from the wire at runtime has to be mapped to a compile-time field description. The message codec ([meta/codec.h](src/iprtl/proto/meta/codec.h)) folds over the fields with `||` and hands the value to the first field whose number matches. Unknown field numbers are skipped.

### Varint and ZigZag

Protobuf stores most integers as **varints**: 7 bits of payload per byte, with the high bit marking that another byte follows. Small numbers need fewer bytes. Negative `int32`/`int64` values always take 10 bytes. For that reason, `sint32`/`sint64` first apply **ZigZag** encoding (`(n << 1) ^ (n >> 63)`), which maps small negative numbers to small unsigned numbers. See [wire/read.h](src/iprtl/proto/wire/read.h), [wire/encode.h](src/iprtl/proto/wire/encode.h) and [scalar_codecs.h](src/iprtl/proto/scalar_codecs.h). The tests in [tests/wire/varint_tests.cpp](tests/wire/varint_tests.cpp) and [tests/proto/message/basic_ints_tests.cpp](tests/proto/message/basic_ints_tests.cpp) show the exact byte sequences.

---

## Library structure

The library has three layers. Each one only uses the layers below it:

```
 generated .proto.h headers
            │  include
            ▼
 meta.h                     encode_message, decode_message, encoded_length,
            │               messages_equal, format_message
            ▼
 meta/   iprtl::proto::meta     messages and fields: ProtoMessage, ProtoField, nested_p,
            │                   their codecs, equality and formatting
            ▼
 proto/  iprtl::proto           one value at a time: proto types (int32_p, string_p,
            │                   enum_p, packed_p, …) and their codecs
            ▼
 wire/   iprtl::proto::wire     raw wire format: varint, i32, i64, len_data, records
```

Three rules keep the layers apart:

- Nothing in `proto/` includes anything from `meta/`. `proto/` does not know that messages exist.
- Each header's namespace matches its folder.
- Generated code only includes `meta.h` and only calls the functions declared there.

### The codec customization point

The layers are connected by one class template, declared in [codec.h](src/iprtl/proto/codec.h):

```cpp
namespace iprtl::proto {
template<typename ProtoT>
struct codec;   // no definition: a type without a specialization is not supported
}
```

Every supported type provides a specialization with the same shape:

```cpp
using value_type = ...;                                         // the C++ value
static std::size_t        length(value_type const&);
static std::span<uint8_t> encode(std::span<uint8_t>, value_type const&);
static ...                decode(...);                          // see below
```

Three free functions forward to it, so callers never name `codec` directly:

```cpp
length<sint32_p>(-1);                  // 1
encode<string_p>(buffer, "hello");     // writes 05 68 65 6c 6c 6f, returns the rest of buffer
decode<sint32_p>(wire::varint{1});     // -1
```

The same three calls also work for a field (`length<Field>(object)`) or a whole message (`length<Proto_Date>(date)`). `decode` has a different signature for each kind:

| Kind | Specialized in | `decode` |
|---|---|---|
| Single value (`int32_p`, `enum_p<E>`, `string_p`, `packed_p<T>`, …) | [scalar_codecs.h](src/iprtl/proto/scalar_codecs.h), [packed_codec.h](src/iprtl/proto/packed_codec.h) | `decode(wire_value) → value` |
| Nested message (`meta::nested_p<M>`) | [meta/codec.h](src/iprtl/proto/meta/codec.h) | `decode(len_data) → message` |
| Field (`meta::ProtoField<…>`) | [meta/codec.h](src/iprtl/proto/meta/codec.h) | `decode(object, wire_value)` stores into the member |
| Message (`meta::ProtoMessage<…>`) | [meta/codec.h](src/iprtl/proto/meta/codec.h) | `decode(buffer, message) → remaining buffer` |

Single values are grouped with constrained partial specializations, for example:

```cpp
template<typename ProtoT>
requires is_encoded_varint<ProtoT>     // int32, int64, uint32, uint64, sint32, sint64, bool
struct codec<ProtoT> { ... };
```

### Why a class template and not overloaded functions

An earlier version used overloaded function templates (`length<T>`, `encode<T>`, `as<T>`) that were selected by concepts. That caused two problems:

- **Name hiding.** Once message overloads moved into `namespace meta`, a call such as `length<P>(v)` inside `meta` found only `meta::length` and stopped looking, so the overloads in `proto` were invisible. Argument-dependent lookup did not help, because the arguments are `int` or user structs.
- **Declaration order.** A field calls the nested-message overload, which calls the message overload, which calls the field overload again. With functions, each one has to be declared before it is used, which needed forward declarations and an include loop between `as.h` and `decode.h`.

The compiler looks up class template specializations when the codec is *used*, which happens in user code after all headers are included. So `meta/` can add `codec<meta::nested_p<M>>` without `proto/` knowing about it, and the order of the specializations does not matter.

### Following one call

Encoding a `DateTime`:

```
encode_message(dt)                                     generated, user namespace
└─ iprtl::proto::encode_message<Proto_DateTime>        meta.h
   └─ codec<ProtoMessage<DateTime, …>>::encode         meta/codec.h   folds over fields
      ├─ codec<ProtoField<…, nested_p<Proto_Date>, …>>  meta/codec.h   tag + value
      │  └─ codec<nested_p<Proto_Date>>::encode         meta/codec.h   length prefix + message
      │     └─ codec<ProtoMessage<Date, …>>::encode     meta/codec.h
      │        └─ codec<ProtoField<…, uint32_p, …>>     meta/codec.h
      │           └─ codec<uint32_p>::encode            scalar_codecs.h
      │              └─ wire::encode(buffer, varint)    wire/encode.h
      └─ codec<ProtoField<…, sint32_p, …>>              …
```

### Adding a new type

To support a new proto type, add a tag type and a `codec` specialization for it. For a single value, put both in `proto/`. For anything that needs to know about messages, put them in `meta/`. No existing code has to change.

---

## Project layout

```
src/iprtl/
├── enums.h, template_string.h      Helpers: reflective enums, compile-time strings
└── proto/
    ├── wire/                       iprtl::proto::wire: the raw wire format
    │   ├── types.h                 varint, i32, i64, len_data, record, tag()
    │   ├── read.h, write.h         read/write a single wire value or record
    │   ├── encode.h, length.h      encode() / length() overloads for wire values
    │   ├── is_correct_length.h     check a buffer before reading
    │   └── wire_exception.h        thrown on malformed input
    ├── proto_wire.h                umbrella header for the wire layer
    │
    ├── types.h                     iprtl::proto: proto type tags (int32_p, …, enum_p, repeated_p, packed_p)
    ├── concepts.h                  groups of proto types (is_encoded_varint, is_packed_p, …)
    ├── codec.h                     codec<T> declaration + length/encode/decode free functions
    ├── scalar_codecs.h             codecs for scalars, enums, string and bytes
    ├── packed_repeated_values.h    iterates over the elements of a packed field
    ├── packed_codec.h              codec for packed_p<T>
    ├── proto_exception.h           thrown on invalid messages
    │
    ├── meta/                       iprtl::proto::meta: messages and fields
    │   ├── fixedstring.h           compile-time string used for names
    │   ├── types.h                 ProtoMessage, ProtoField, nested_p
    │   ├── concepts.h              is_proto_message_p, is_proto_field_p, is_nested_p
    │   ├── codec.h                 codecs for messages, nested messages and fields
    │   ├── equal.h                 field-wise comparison
    │   └── types_format.h          {fmt} formatting of fields and messages
    ├── meta.h                      entry point for generated code
    │
    ├── compiler/                   protopiler: tokenizer, parser, AST and C++ code generator
    └── tools/                      wire-dump: prints the raw records of a binary message
src/examples/datetime/              Example program using generated code
tests/
├── wire/                           wire layer
├── proto/                          single-value codecs, fields and packed values
├── proto/message/                  whole messages, using headers generated from .proto files
├── tokenizer/, compiler/           protopiler front end
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

---

## License

[MIT](LICENSE). Code generated by `protopiler` may be used without restriction.
