#pragma once

#include <istream>
#include <optional>
#include <stdexcept>
#include "proto_tokenizer.h"
#include "proto_ast.h"

namespace iprtl::proto::compiler {
class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class ProtoParser {
public:
    static ProtoFile parse(std::istream& input);

private:
    ProtoTokenizer tokenizer;
    std::optional<Token> current_token;

    explicit ProtoParser(std::istream& input);
    ProtoFile parse_impl();

    void advance();
    bool match(std::string_view text);
    void expect(std::string_view text);
    bool is(std::string_view text) { return current_token && current_token->text == text;}
    std::string identifier() { if(current_token && current_token->kind == TokenKind::Identifier) { return current_token->text; } throw ParseError("Expected identifier"); }
    std::optional<Token> peek() const;

    void parse_syntax(ProtoFile& proto);
    void parse_package(ProtoFile& proto);
    void parse_import(ProtoFile& proto);
    std::shared_ptr<Message> parse_message();
    Field parse_field();
    Enum parse_enum();
    std::string parse_type();
};
} // namespace iprtl::proto::compiler