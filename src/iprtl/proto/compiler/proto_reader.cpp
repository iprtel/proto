#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>


#include <fmt/printf.h>
#include <fmt/ostream.h>
#include <fmt/args.h>
#include <argparse/argparse.hpp>

#include "proto_parser.h"
#include "proto_ast.h"
using iprtl::proto::compiler::Field;
using iprtl::proto::compiler::FieldType;
using iprtl::proto::compiler::ProtoFile;
using iprtl::proto::compiler::ProtoParser;

std::string map_cpp_type(std::string const& name) {
    auto type = iprtl::proto::compiler::type_from_string(name);
    switch(type) {
        case FieldType::Double: return "double";
        case FieldType::Float: return "float";
        case FieldType::Int64: return "int64_t";
        case FieldType::Uint64: return "uint64_t";
        case FieldType::Int32: return "int32_t";
        case FieldType::Uint32: return "uint32_t";
        case FieldType::Fixed64: return "uint64_t";
        case FieldType::Fixed32: return "uint32_t";
        case FieldType::Bool: return "bool";
        case FieldType::String: return "std::string";
        case FieldType::Group: return "message";
        case FieldType::Bytes: return "std::vector<uint8_t>";
        case FieldType::Sfixed32: return "int32_t";
        case FieldType::Sfixed64: return "int64_t";
        case FieldType::Sint32: return "int32_t";
        case FieldType::Sint64: return "int64_t";
        case FieldType::Name: break;
    }
    return name; // For Message, Enum we return the type name as is
}

std::string get_cpp_type(Field const& field) {
    auto mapped = map_cpp_type(field.type);
    if (field.repeated) {
        return fmt::format("std::vector<{}>", mapped);
    }
    return mapped;
}



void print_ast(const ProtoFile& proto, int indent = 0) {
    auto ind = [indent]() { return std::string(indent * 2, ' '); };

    fmt::print("{}ProtoFile:\n", ind());
    fmt::print("{}  syntax: {}\n", ind(), proto.syntax);
    if (!proto.package.empty()) {
        fmt::print("{}  package: {}\n", ind(), proto.package);
    }

    if (!proto.imports.empty()) {
        fmt::print("{}  imports:\n", ind());
        for (const auto& imp : proto.imports) {
            fmt::print("{}    - {}\n", ind(), imp);
        }
    }

    if (!proto.enums.empty()) {
        fmt::print("{}  enums:\n", ind());
        for (const auto& enum_def : proto.enums) {
            fmt::print("{}    enum {}:\n", ind(), enum_def.name);
            for (const auto& val : enum_def.values) {
                fmt::print("{}      {} = {}\n", ind(), val.name, val.number);
            }
        }
    }

    if (!proto.messages.empty()) {
        fmt::print("{}  messages:\n", ind());
        for (const auto& msg : proto.messages) {
            fmt::print("{}    message {}:\n", ind(), msg->name);
            for (const auto& field : msg->fields) {
                fmt::print("{}      {}{} {} = {}\n", ind(),
                          field.repeated ? "repeated " : "",
                          field.type, field.name, field.number);
            }
            if (!msg->nested_messages.empty()) {
                fmt::print("{}      nested_messages:\n", ind());
                for (const auto& nmsg : msg->nested_messages) {
                    fmt::print("{}        message {}:\n", ind(), nmsg->name);
                    for (const auto& field : nmsg->fields) {
                        fmt::print("{}          {}{} {} = {}\n", ind(),
                                  field.repeated ? "repeated " : "",
                                  field.type, field.name, field.number);
                    }
                }
            }
        }
    }
}


std::string make_namespace(const std::string& package) {
    std::string ns;
    for (char c : package) {
        if (c == '.') {
            ns += "::";
        } else {
            ns += c;
        }
    }
    return ns;
}

struct Generator{
    Generator(std::ofstream& out_file, iprtl::proto::compiler::ProtoFile proto, std::string base_ns)
        : out_file(out_file), proto(proto), base_ns(base_ns) {}
    std::ofstream& out_file;
    iprtl::proto::compiler::ProtoFile proto;
    std::string base_ns;
    std::string map_proto_type(std::string const& name){
        auto type = iprtl::proto::compiler::type_from_string(name);
        switch(type) {
            case FieldType::Double: return "iprtl::proto::double_p";
            case FieldType::Float: return "iprtl::proto::float_p";
            case FieldType::Int64: return "iprtl::proto::int64_p";
            case FieldType::Uint64: return "iprtl::proto::uint64_p";
            case FieldType::Int32: return "iprtl::proto::int32_p";
            case FieldType::Uint32: return "iprtl::proto::uint32_p";
            case FieldType::Fixed64: return "iprtl::proto::fixed64_p";
            case FieldType::Fixed32: return "iprtl::proto::fixed32_p";
            case FieldType::Bool: return "iprtl::proto::bool_p";
            case FieldType::String: return "iprtl::proto::string_p";
            case FieldType::Group: return "group";
            case FieldType::Bytes: return "iprtl::proto::bytes_p";
            case FieldType::Sfixed32: return "iprtl::proto::sfixed32_p";
            case FieldType::Sfixed64: return "iprtl::proto::sfixed64_p";
            case FieldType::Sint32: return "iprtl::proto::sint32_p";
            case FieldType::Sint64: return "iprtl::proto::sint64_p";
            case FieldType::Name: break; // an enum or message, resolved below
        }

        auto found_enum = std::find_if(proto.enums.begin(), proto.enums.end(),
            [&name](const iprtl::proto::compiler::Enum& e) { return e.name == name; });
        if (found_enum != proto.enums.end()) {
            return "iprtl::proto::enum_p<" + name + ">";
        }
        auto found_nested = std::find_if(proto.messages.begin(), proto.messages.end(),
            [&name](const std::shared_ptr<iprtl::proto::compiler::Message>& m) { return m->name == name; });
        if (found_nested != proto.messages.end()) {
            return "iprtl::proto::meta::nested_p<Proto_" + name + ">";
        }

        return name;
    }


    std::string get_proto_type(Field const& field) {
        auto mapped = map_proto_type(field.type);
        if (field.repeated) {
            if (field.packed) {
                return fmt::format("iprtl::proto::packed_p<{}>", mapped);
            } else {
                return fmt::format("iprtl::proto::repeated_p<{}>", mapped);
            }
        }
        return std::string(mapped);
    }



    void generate() {
        fmt::print(out_file,"#include \"iprtl/proto/meta.h\"\n");
        if (not proto.package.empty()) {
            if (not base_ns.empty()) {
                base_ns += "::";
            }
            fmt::print(out_file,"namespace {}{} {{\n", base_ns, make_namespace(proto.package));
        } else {
            fmt::print(out_file,"namespace {} {{\n", base_ns);
        }

        auto enums = proto.enums;
        for (const auto& enum_def : enums) {
            fmt::print(out_file,"enum class {} {{\n", enum_def.name);
            for (const auto& val : enum_def.values) {
                fmt::print(out_file,"    {} = {},\n", val.name, val.number);
            }
            fmt::print(out_file,"}};\n");
                        fmt::print(out_file,
                "inline auto format_as({meta_enum_name} const& v) {{return static_cast<int>(v);}}\n",
                fmt::arg("meta_enum_name",enum_def.name));
        }

        auto messages = proto.messages;
        for (const auto& msg : messages) {
            fmt::print(out_file,"struct {} {{\n",msg->name);
            for (const auto& field : msg->fields) {
                fmt::print(out_file,"    {} {}{{}};\n", get_cpp_type(field), field.name);
            }
            fmt::print(out_file,"}};\n");

            auto meta_msg_name = fmt::format("Proto_{msg_name}", fmt::arg("msg_name",msg->name));
            fmt::print(out_file, "using {meta_msg_name} = iprtl::proto::meta::ProtoMessage<{msg_name}, \"{msg_name}\"", fmt::arg("meta_msg_name",meta_msg_name),fmt::arg("msg_name",msg->name) );
            for (const auto& field : msg->fields) {
                auto proto_field_definition = fmt::format(",\n    iprtl::proto::meta::ProtoField<{msg_name},{field_index},{field_proto_t},\"{field_name}\",&{msg_name}::{field_name}>", fmt::arg("msg_name",msg->name), fmt::arg("field_name",field.name), fmt::arg("field_index",field.number), fmt::arg("field_proto_t",get_proto_type(field)));
                fmt::print(out_file,"{}", proto_field_definition);
            }
            fmt::print(out_file,">;\n");

            fmt::print(out_file,
                "inline auto format_as({msg_name} const& v) {{return iprtl::proto::format_message<{meta_msg_name}>(v);}}\n",
                fmt::arg("meta_msg_name",meta_msg_name),fmt::arg("msg_name",msg->name));
            fmt::print(out_file,
                "inline auto decode_message({msg_name} & v, std::span<const uint8_t> buffer) {{return iprtl::proto::decode_message<{meta_msg_name}>(v,buffer);}}\n",
                fmt::arg("meta_msg_name",meta_msg_name),fmt::arg("msg_name",msg->name));
            fmt::print(out_file,
                "inline auto encode_message({msg_name} const& v, std::span<uint8_t> buffer) {{return iprtl::proto::encode_message<{meta_msg_name}>(v,buffer);}}\n",
                fmt::arg("meta_msg_name",meta_msg_name),fmt::arg("msg_name",msg->name));
            fmt::print(out_file,
                "inline std::vector<uint8_t> encode_message({msg_name} const& v) {{return iprtl::proto::encode_message<{meta_msg_name}>(v);}}\n",
                fmt::arg("meta_msg_name",meta_msg_name),fmt::arg("msg_name",msg->name));
            fmt::print(out_file,
                    "inline bool operator==(const {msg_name}& lhs, const {msg_name}& rhs) {{ return iprtl::proto::messages_equal<{meta_msg_name}>(lhs, rhs); }}\n", fmt::arg("msg_name",msg->name), fmt::arg("meta_msg_name",meta_msg_name));
            fmt::print(out_file,
                    "inline std::size_t encoded_length(const {msg_name}& msg) {{ return iprtl::proto::encoded_length<{meta_msg_name}>(msg); }}\n", fmt::arg("msg_name",msg->name), fmt::arg("meta_msg_name",meta_msg_name));
        }
        fmt::print(out_file,"}}\n");

    }

};



int main(int argc, char** argv) {
    argparse::ArgumentParser program("proto-parser");

    program.add_argument("--base_ns").default_value(std::string{""});
    program.add_argument("--out-dir").default_value(std::string{"."});
    program.add_argument("files").default_value(std::vector<std::string>{"test.proto"}).remaining();

    try {
      program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
      fmt::print("ERROR: {}\n", err.what());
      std::exit(1);
    }

    std::vector<std::string> files;
    files = program.get<std::vector<std::string>>("files");
    if (files.empty()) {
        fmt::print("No input file given.\n");
        std::exit(1);
    }
    auto out_dir=program.get<std::string>("--out-dir");
    std::filesystem::path out_dir_path(out_dir);
    if (not std::filesystem::exists(out_dir_path) &&
        not std::filesystem::create_directories(out_dir_path)) {
        fmt::print("Failed to create output directory: {}\n", out_dir);
        return 1;
    }

    auto const& proto_path = files[0];
    std::filesystem::path proto_file_path(proto_path);
    if (!std::filesystem::exists(proto_file_path)) {
        fmt::print("File not found: {}\n", proto_path);
        return 1;
    }

    std::ifstream input{proto_file_path};
    std::filesystem::path out_dir_name{out_dir};
    auto out_file_name = out_dir_name /= proto_file_path.filename();
    out_file_name.replace_extension(".proto.h");
    fmt::print("Out file: {}\n", out_file_name.string());
    std::ofstream out_file{out_file_name,std::ios::out};
    try {
        auto proto = iprtl::proto::compiler::ProtoParser::parse(input);

        fmt::print("Successfully parsed: {}\n\n", proto_path);
        print_ast(proto);

        Generator generator(out_file, proto, program.get<std::string>("--base_ns"));
        generator.generate();
        return 0;

    } catch (const iprtl::proto::compiler::ParseError& e) {
        fmt::print("Parse error: {}\n", e.what());
        return 1;
    } catch (const std::exception& e) {
        fmt::print("Error: {}\n", e.what());
        return 1;
    }
}

