#pragma once
#ifndef LEXER_TOKEN_HPP
#define LEXER_TOKEN_HPP

#include <string>
#include <utility>

namespace lexer {
enum token_type {
    AUTO = 1,
    BREAK,
    CASE,
    CHAR,
    CONST,
    CONTINUE,
    DEFAULT,
    DO,
    DOUBLE,
    ELSE,
    ENUM,
    EXTERN,
    FLOAT,
    FOR,
    GOTO,
    IF,
    INT,
    LONG,
    REGISTER,
    RETURN,
    SHORT,
    SIGNED,
    SIZEOF,
    STATIC,
    STRUCT,
    SWITCH,
    TYPEDEF,
    UNION,
    UNSIGNED,
    VOID,
    VOLATILE,
    WHILE,
    MINUS,
    MINUS_MINUS,
    MINUS_EQUAL,
    ARROW,
    BANG,
    BANG_EQUAL,
    PERCENT,
    PERCENT_EQUAL,
    AMPERSAND,
    AMPERSAND_AMPERSAND,
    AMPERSAND_EQUAL,
    LPAREN,
    RPAREN,
    ASTERISK,
    ASTERISK_EQUAL,
    COMMA,
    DOT,
    SLASH,
    SLASH_EQUAL,
    COLON,
    SEMICOLON,
    QUESTION,
    LBRACKET,
    RBRACKET,
    CARET,
    CARET_EQUAL,
    LBRACE,
    PIPE,
    PIPE_PIPE,
    PIPE_EQUAL,
    RBRACE,
    TILDE,
    PLUS,
    PLUS_PLUS,
    PLUS_EQUAL,
    LESS,
    LESS_LESS,
    LESS_LESS_EQUAL,
    LESS_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    GREATER_GREATER,
    GREATER_GREATER_EQUAL,
    QUOTE,
    COMMENT,
    NUMBER,
    IDENTIFIER,
    WHITESPACE
};

struct token {
    token_type type;
    std::string value;
    std::size_t line;
    std::size_t column;

    token(const token_type type, std::string value, const std::size_t line, const std::size_t column)
        : type(type), value(std::move(value)), line(line), column(column) {}

    explicit token(std::string value) : type(static_cast<token_type>(0)), value(std::move(value)), line(0), column(0) {}

    explicit operator std::string() const {
        return value;
    }
};
} // namespace lexer::token

#endif // LEXER_TOKEN_HPP