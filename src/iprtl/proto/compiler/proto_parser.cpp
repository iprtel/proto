#include "proto_parser.h"
#include <fmt/core.h>

namespace iprtl::proto::compiler {

namespace {
    bool is_packable_type(const std::string& type) {
    return type == "int32" || type == "int64" || type == "uint32" || type == "uint64" || type == "sint32" || type == "sint64" ||
           type == "fixed32" || type == "fixed64" || type == "sfixed32" || type == "sfixed64" ||
           type == "float" || type == "double" || type == "bool";
}

}

ProtoFile ProtoParser::parse(std::istream& input) {
    ProtoParser parser(input);
    return parser.parse_impl();
}

ProtoParser::ProtoParser(std::istream& input) : tokenizer(input) {
    advance();
}

ProtoFile ProtoParser::parse_impl() {
    ProtoFile proto{"proto3", "", {}, {}, {}};

    // Parse syntax declaration
    if (current_token && current_token->kind == TokenKind::Identifier && current_token->text == "syntax") {
        parse_syntax(proto);
    }

    // Parse top-level declarations
    while (current_token && current_token->kind != TokenKind::Eof) {
        if (current_token->kind == TokenKind::Identifier) {
            if (current_token->text == "package") {
                parse_package(proto);
            } else if (current_token->text == "import") {
                parse_import(proto);
            } else if (current_token->text == "message") {
                advance(); // skip 'message'
                auto msg = parse_message();
                if (msg) {
                    proto.messages.push_back(msg);
                }
            } else if (current_token->text == "enum") {
                advance(); // skip 'enum'
                proto.enums.push_back(parse_enum());
            } else {
                advance();
            }
        } else {
            advance();
        }
    }

    return proto;
}

void ProtoParser::advance() {
    current_token = tokenizer.next_token();
}

bool ProtoParser::match(std::string_view text) {
    if (current_token && current_token->text == text) {
        advance();
        return true;
    }
    return false;
}

void ProtoParser::expect(std::string_view text) {
    if (!current_token || current_token->text != text) {
        throw ParseError(fmt::format("Expected '{}', got '{}'", text,
                                     current_token ? current_token->text : "EOF"));
    }
    advance();
}

std::optional<Token> ProtoParser::peek() const {
    return current_token;
}

void ProtoParser::parse_syntax(ProtoFile& proto) {
    expect("syntax");
    expect("=");
    if (current_token && (current_token->text == "proto2" || current_token->text == "proto3")) {
        proto.syntax = current_token->text;
        advance();
    } else {
        throw ParseError("Expected 'proto2' or 'proto3'");
    }
    expect(";");
}

void ProtoParser::parse_package(ProtoFile& proto) {
    expect("package");
    proto.package = identifier();
    advance();
    while(is(".")) {
        advance(); // skip '.'
        proto.package += "." + identifier();
        advance();
    }
    expect(";");
}

void ProtoParser::parse_import(ProtoFile& proto) {
    expect("import");
    if (current_token && current_token->kind == TokenKind::String) {
        proto.imports.push_back(current_token->text);
        advance();
    } else {
        throw ParseError("Expected import path");
    }
    expect(";");
}

std::shared_ptr<Message> ProtoParser::parse_message() {
    if (!current_token || current_token->kind != TokenKind::Identifier) {
        throw ParseError("Expected message name");
    }

    auto msg = std::make_shared<Message>();
    msg->name = current_token->text;
    advance();

    expect("{");

    while (current_token && current_token->text != "}") {
        if (!current_token || current_token->kind == TokenKind::Eof) {
            throw ParseError("Unexpected EOF in message body");
        }

        if (current_token->kind == TokenKind::Identifier) {
            if (current_token->text == "message") {
                advance(); // skip 'message'
                auto nested = parse_message();
                if (nested) {
                    msg->nested_messages.push_back(nested);
                }
            } else if (current_token->text == "enum") {
                advance(); // skip 'enum'
                msg->nested_enums.push_back(parse_enum());
            } else {
                // Anything else starts a field: a modifier or a type name
                Field field = parse_field();
                msg->fields.push_back(field);
            }
        } else {
            advance();
        }
    }

    expect("}");
    return msg;
}

Field ProtoParser::parse_field() {
    Field field;

    // Check for modifiers
    if (current_token && current_token->text == "repeated") {
        field.repeated = true;
        advance();
    } else if (current_token && current_token->text == "optional") {
        advance();
    } else if (current_token && current_token->text == "required") {
        advance();
    }

    // Parse type (may be qualified, e.g. "other.pkg.Type")
    field.type = parse_type();
    field.packed = field.repeated && is_packable_type(field.type);

    // Parse field name
    if (current_token && current_token->kind == TokenKind::Identifier) {
        field.name = current_token->text;
        advance();
    } else {
        throw ParseError("Expected field name");
    }

    expect("=");

    // Parse field number
    if (current_token && current_token->kind == TokenKind::Number) {
        try {
            field.number = std::stoi(current_token->text);
        } catch (...) {
            throw ParseError("Invalid field number");
        }
        advance();
    } else {
        throw ParseError("Expected field number");
    }

    // Parse optional field options
    if (current_token && current_token->text == "[") {
        advance(); // skip '['
        while (current_token && current_token->text != "]") {
            fmt::print("Packed option: {} {}\n", field.name, current_token->text);
            if (current_token->text == "packed") {
                field.packed = true;
                advance();
                if (match("=")) {
                    if (current_token){
                        field.packed = current_token->text == "true";
                        fmt::print("Packed option: {} {}\n", field.packed, current_token->text);
                    }
                    advance();
                }
            } else {
                advance();
            }
        }
        expect("]");
    }

    expect(";");
    return field;
}

Enum ProtoParser::parse_enum() {
    Enum enum_def;

    if (!current_token || current_token->kind != TokenKind::Identifier) {
        throw ParseError("Expected enum name");
    }

    enum_def.name = current_token->text;
    advance();

    expect("{");

    while (current_token && current_token->text != "}") {
        if (!current_token || current_token->kind == TokenKind::Eof) {
            throw ParseError("Unexpected EOF in enum body");
        }

        if (current_token->kind == TokenKind::Identifier) {
            EnumValue val;
            val.name = current_token->text;
            advance();

            expect("=");

            if (current_token && current_token->kind == TokenKind::Number) {
                try {
                    val.number = std::stoi(current_token->text);
                } catch (...) {
                    throw ParseError("Invalid enum number");
                }
                advance();
            }

            expect(";");
            enum_def.values.push_back(val);
        } else {
            advance();
        }
    }

    expect("}");
    return enum_def;
}

std::string ProtoParser::parse_type() {
    std::string type;
    // A leading '.' marks a fully qualified name, e.g. ".pkg.Type"
    if (is(".")) {
        type = ".";
        advance();
    }
    type += identifier();
    advance();
    while (is(".")) {
        advance(); // skip '.'
        type += "." + identifier();
        advance();
    }
    return type;
}


} // namespace iprtl::proto::compiler