#pragma once

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace lexer {
class token;
}

enum class token_type {
    UNKNOWN = 0,
    INT,
    DOUBLE,
    LONG,
    IF,
    ELSE,
    FOR,
    WHILE,
    LPAR,
    RPAR,
    SEMI,
    LBRACE,
    RBRACE,
    PLUS,
    MINUS,
    MULT,
    DIV,
    LT,
    LE,
    GT,
    GE,
    EQ,
    ASSIGN,
    ID,
    INTNUM,
    DOUBLENUM,
    WHITESPACE,
    BITAND, // &
    BITOR,  // |
    BITXOR, // ^
    LOGAND, // &&
    LOGOR,  // ||
    COMMA,  // ,
    STRING, // "string"
    BITNOT, // ~
    LOGNOT, // !
    NE,     // !=
    COMMENT
};

extern const std::unordered_set<std::string> terminals;
extern std::map<std::string, std::string> strings;

std::string process_string_literal(const std::string& literal);
std::string trim_zero(std::string s);

std::vector<lexer::token> lex(const std::string& str);