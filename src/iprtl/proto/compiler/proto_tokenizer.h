#pragma once

#include <istream>
#include <optional>
#include <string>
#include <coroutine>

#include "iprtl/enums.h"

namespace iprtl::proto::compiler {
ENUMS(TokenKind, int,
    Identifier,
    Number,
    String,
    Symbol,
    Eof
);

struct Token {
    TokenKind kind;
    std::string text;
    size_t line;
    size_t column;
};

template<typename T>
class Generator
{
public:
    struct promise_type
    {
        Generator<T> get_return_object()
        {
            return Generator{Handle::from_promise(*this)};
        }
        static std::suspend_always initial_suspend() noexcept
        {
            return {};
        }
        static std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        std::suspend_always yield_value(T value) noexcept
        {
            current_value = std::move(value);
            return {};
        }
        void return_void() { current_value = std::nullopt; }
        // Disallow co_await in generator coroutines.
        void await_transform() = delete;
        [[noreturn]]
        static void unhandled_exception() { throw; }
        std::optional<T> current_value;
    };
    using Handle = std::coroutine_handle<promise_type>;
    explicit Generator(const Handle coroutine) :
        m_coroutine{coroutine}
    {}
    Generator() = default;
    ~Generator()
    {
        if (m_coroutine)
            m_coroutine.destroy();
    }
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&& other) noexcept :
        m_coroutine{other.m_coroutine}
    {
        other.m_coroutine = {};
    }
    Generator& operator=(Generator&& other) noexcept
    {
        if (this != &other)
        {
            if (m_coroutine)
                m_coroutine.destroy();
            m_coroutine = other.m_coroutine;
            other.m_coroutine = {};
        }
        return *this;
    }

    bool next()
    {
        if (m_coroutine && !m_coroutine.done())
        {
            m_coroutine.resume();
        }
        return m_coroutine && !m_coroutine.done();
    }
    std::optional<T> value() const {
        if (m_coroutine)
            return m_coroutine.promise().current_value;
        return std::nullopt;
    }
private:
    Handle m_coroutine;
};



class ProtoTokenizer {
public:
    explicit ProtoTokenizer(std::istream& is);
    std::optional<Token> next_token();
    bool eof() const;

private:
    std::istream& input;
    Generator<Token> generator;
    bool done_ = false;
    std::optional<Token> current_token;
    size_t current_line{1};
    size_t current_column{1};

    Generator<Token> tokenize();
    void skip_whitespace_and_comments();
    char peek_char();
    char peek_next();
    std::string read_identifier();
    std::string read_number();
    std::string read_string();
    std::string read_symbol();
};
}
