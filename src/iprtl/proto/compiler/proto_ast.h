#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace iprtl::proto::compiler {
// Forward declarations
struct Message;
struct Enum;
struct Field;

ENUMS(FieldType, uint8_t,
    Double, Float, Int32, Int64, Uint32, Uint64, Sint32, Sint64, Fixed32, Fixed64,
    Sfixed32, Sfixed64, Bool, String, Bytes, Group, Name)

inline FieldType type_from_string(const std::string& str){
    if (str == "double") return FieldType::Double;
    if (str == "float") return FieldType::Float;
    if (str == "int32") return FieldType::Int32;
    if (str == "int64") return FieldType::Int64;
    if (str == "uint32") return FieldType::Uint32;
    if (str == "uint64") return FieldType::Uint64;
    if (str == "sint32") return FieldType::Sint32;
    if (str == "sint64") return FieldType::Sint64;
    if (str == "fixed32") return FieldType::Fixed32;
    if (str == "fixed64") return FieldType::Fixed64;
    if (str == "sfixed32") return FieldType::Sfixed32;
    if (str == "sfixed64") return FieldType::Sfixed64;
    if (str == "bool") return FieldType::Bool;
    if (str == "string") return FieldType::String;
    if (str == "bytes") return FieldType::Bytes;

    return FieldType::Name; // For Message, Enum
}


// Field definition
struct Field {
    std::string name;
    std::string type;
    std::string enumtype_name; // For message and enum types
    int number = 0;
    bool repeated = false;
    bool packed = false;
};

// Enum value definition
struct EnumValue {
    std::string name;
    int number = 0;
};

// Enum definition
struct Enum {
    std::string name;
    std::vector<EnumValue> values;
};

// Message definition
struct Message {
    std::string name;
    std::vector<Field> fields;
    std::vector<std::shared_ptr<Message>> nested_messages;
    std::vector<Enum> nested_enums;
};

// Proto file AST
struct ProtoFile {
    std::string syntax;
    std::string package;
    std::vector<std::string> imports;
    std::vector<std::shared_ptr<Message>> messages;
    std::vector<Enum> enums;
};
} // namespace iprtl::proto::compiler