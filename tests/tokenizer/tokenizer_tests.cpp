#include <gtest/gtest.h>
#include <sstream>
#include <vector>
#include "iprtl/proto/compiler/proto_tokenizer.h"

TEST(ProtoTokenizerTest, SimpleTokens) {
    std::istringstream iss("message Test { int32 id = 1; }");
    iprtl::proto::compiler::ProtoTokenizer tokenizer(iss);

    std::vector<iprtl::proto::compiler::Token> tokens;
    while (auto token = tokenizer.next_token()) {
        if (token->kind == iprtl::proto::compiler::TokenKind::Eof) break;
        tokens.push_back(*token);
    }

    ASSERT_EQ(tokens.size(), 9);
    EXPECT_EQ(tokens[0].kind, iprtl::proto::compiler::TokenKind::Identifier);
    EXPECT_EQ(tokens[0].text, "message");
    EXPECT_EQ(tokens[1].kind, iprtl::proto::compiler::TokenKind::Identifier);
    EXPECT_EQ(tokens[1].text, "Test");
    EXPECT_EQ(tokens[2].kind, iprtl::proto::compiler::TokenKind::Symbol);
    EXPECT_EQ(tokens[2].text, "{");
    EXPECT_EQ(tokens[3].kind, iprtl::proto::compiler::TokenKind::Identifier);
    EXPECT_EQ(tokens[3].text, "int32");
    EXPECT_EQ(tokens[4].kind, iprtl::proto::compiler::TokenKind::Identifier);
    EXPECT_EQ(tokens[4].text, "id");
    EXPECT_EQ(tokens[5].kind, iprtl::proto::compiler::TokenKind::Symbol);
    EXPECT_EQ(tokens[5].text, "=");
    EXPECT_EQ(tokens[6].kind, iprtl::proto::compiler::TokenKind::Number);
    EXPECT_EQ(tokens[6].text, "1");
    EXPECT_EQ(tokens[7].kind, iprtl::proto::compiler::TokenKind::Symbol);
    EXPECT_EQ(tokens[7].text, ";");
    EXPECT_EQ(tokens[8].kind, iprtl::proto::compiler::TokenKind::Symbol);
    EXPECT_EQ(tokens[8].text, "}");
}

TEST(ProtoTokenizerTest, StringLiteral) {
    std::istringstream iss("\"hello world\"");
    iprtl::proto::compiler::ProtoTokenizer tokenizer(iss);

    std::vector<iprtl::proto::compiler::Token> tokens;
    while (auto token = tokenizer.next_token()) {
        if (token->kind == iprtl::proto::compiler::TokenKind::Eof) break;
        tokens.push_back(*token);
    }

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].kind, iprtl::proto::compiler::TokenKind::String);
    EXPECT_EQ(tokens[0].text, "hello world");
}

TEST(ProtoTokenizerTest, CommentsSkipped) {
    std::istringstream iss("message // comment\nTest /* block */ { }");
    iprtl::proto::compiler::ProtoTokenizer tokenizer(iss);

    std::vector<iprtl::proto::compiler::Token> tokens;
    while (auto token = tokenizer.next_token()) {
        if (token->kind == iprtl::proto::compiler::TokenKind::Eof) break;
        tokens.push_back(*token);
    }

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].text, "message");
    EXPECT_EQ(tokens[1].text, "Test");
    EXPECT_EQ(tokens[2].text, "{");
    EXPECT_EQ(tokens[3].text, "}");
}

TEST(ProtoTokenizerTest, WhitespaceSkipped) {
    std::istringstream iss("  \t\n  message   Test  \n  ");
    iprtl::proto::compiler::ProtoTokenizer tokenizer(iss);

    std::vector<iprtl::proto::compiler::Token> tokens;
    while (auto token = tokenizer.next_token()) {
        tokens.push_back(*token);
    }

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].text, "message");
    EXPECT_EQ(tokens[1].text, "Test");
    EXPECT_EQ(tokens[2].kind, iprtl::proto::compiler::TokenKind::Eof);
}

TEST(ProtoTokenizerTest, EmptyInput) {
    std::istringstream iss("");
    iprtl::proto::compiler::ProtoTokenizer tokenizer(iss);

    auto token = tokenizer.next_token();
    ASSERT_TRUE(token);
    EXPECT_EQ(token->kind, iprtl::proto::compiler::TokenKind::Eof);
}