#include "proto_tokenizer.h"

#include <cctype>
#include <iostream>
#include <sstream>

namespace iprtl::proto::compiler {
namespace {
    bool is_identifier_start(char c) {
        return std::isalpha(c) || c == '_';
    }

    bool is_identifier_char(char c) {
        return std::isalnum(c) || c == '_';
    }

    bool is_symbol(char c) {
        return c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')' || c == ';' || c == '=' || c == '<' || c == '>' || c == ',' || c == '.' || c == ':';
    }
} // namespace


ProtoTokenizer::ProtoTokenizer(std::istream& is) : input(is), generator(tokenize()) {}

std::optional<Token> ProtoTokenizer::next_token() {
    if (not done_ && generator.next()) {
        current_token = generator.value();
        done_ = (current_token->kind == TokenKind::Eof);
    } else if (not done_) {
        current_token = Token{TokenKind::Eof, "", 0, 0};
        done_ = true;
        return current_token;
    }
    if (not done_) {
        return current_token;
    } else {
        return std::nullopt;
    }
}

bool ProtoTokenizer::eof() const {
    return done_;
}

Generator<Token> ProtoTokenizer::tokenize() {
    while (true) {
        skip_whitespace_and_comments();
        if (input.eof()) {
            co_return;
        }
        char c = peek_char();
        if (is_identifier_start(c)) {
            size_t start_line = current_line;
            size_t start_col = current_column;
            std::string id = read_identifier();
            co_yield Token{TokenKind::Identifier, id, start_line, start_col};
        } else if (std::isdigit(c) || c == '-') {
            size_t start_line = current_line;
            size_t start_col = current_column;
            std::string num = read_number();
            co_yield Token{TokenKind::Number, num, start_line, start_col};
        } else if (c == '"') {
            size_t start_line = current_line;
            size_t start_col = current_column;
            std::string str = read_string();
            co_yield Token{TokenKind::String, str, start_line, start_col};
        } else if (is_symbol(c)) {
            size_t start_line = current_line;
            size_t start_col = current_column;
            std::string sym = read_symbol();
            co_yield Token{TokenKind::Symbol, sym, start_line, start_col};
        } else {
            // Invalid character, skip for now
            input.get();
            current_column++;
        }
    }
}

void ProtoTokenizer::skip_whitespace_and_comments() {
    while (!input.eof()) {
        char c = peek_char();
        if (std::isspace(c)) {
            input.get();
            if (c == '\n') {
                current_line++;
                current_column = 1;
            } else {
                current_column++;
            }
        } else if (c == '/' && peek_next() == '/') {
            // Line comment
            while (!input.eof() && input.get() != '\n');
            current_line++;
            current_column = 1;
        } else if (c == '/' && peek_next() == '*') {
            // Block comment
            input.get(); // consume '/'
            input.get(); // consume '*'
            current_column += 2;
            while (!input.eof()) {
                char c1 = input.get();
                if (c1 == '\n') {
                    current_line++;
                    current_column = 1;
                } else {
                    current_column++;
                }
                if (c1 == '*' && peek_char() == '/') {
                    input.get(); // consume '/'
                    current_column++;
                    break;
                }
            }
        } else {
            break;
        }
    }
}

char ProtoTokenizer::peek_char() {
    return input.peek();
}

char ProtoTokenizer::peek_next() {
    input.get();
    char c = input.peek();
    input.unget();
    return c;
}

std::string ProtoTokenizer::read_identifier() {
    std::string result;
    while (!input.eof() && is_identifier_char(peek_char())) {
        char c = input.get();
        result += c;
        current_column++;
    }
    return result;
}

std::string ProtoTokenizer::read_number() {
    std::string result;
    while (!input.eof() && (std::isdigit(peek_char()) || peek_char() == '.' || peek_char() == '-' || peek_char() == '+')) {
        char c = input.get();
        result += c;
        current_column++;
    }
    return result;
}

std::string ProtoTokenizer::read_string() {
    std::string result;
    input.get(); // consume opening "
    current_column++;
    while (!input.eof()) {
        char c = input.get();
        current_column++;
        if (c == '"') {
            break;
        } else if (c == '\\') {
            // Handle escape
            if (!input.eof()) {
                char next = input.get();
                current_column++;
                if (next == 'n') result += '\n';
                else if (next == 't') result += '\t';
                else if (next == '"') result += '"';
                else if (next == '\\') result += '\\';
                else result += next; // unknown escape, just add
            }
        } else {
            result += c;
        }
    }
    return result;
}

std::string ProtoTokenizer::read_symbol() {
    char c = input.get();
    current_column++;
    return std::string(1, c);
}

} // namespace iprtl::proto::compiler