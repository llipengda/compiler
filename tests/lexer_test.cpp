#include "lexer/lexer.hpp"

#include <format>
#include <gtest/gtest.h>

enum class token_type {
    INT,
    REAL,
    IF,
    THEN,
    ELSE,
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
    REALNUM,
    WHITESPACE
};

const lexer::lexer::input_keywords_t<token_type> keywords = {
    {"int", token_type::INT, "int"},
    {"real", token_type::REAL, "real"},
    {"if", token_type::IF, "if"},
    {"then", token_type::THEN, "then"},
    {"else", token_type::ELSE, "else"},
    {"\\(", token_type::LPAR, "("},
    {"\\)", token_type::RPAR, ")"},
    {";", token_type::SEMI, ";"},
    {"\\{", token_type::LBRACE, "{"},
    {"\\}", token_type::RBRACE, "}"},
    {"\\+", token_type::PLUS, "+"},
    {"-", token_type::MINUS, "-"},
    {"\\*", token_type::MULT, "*"},
    {"/", token_type::DIV, "/"},
    {"<=", token_type::LE, "<="},
    {"<", token_type::LT, "<"},
    {">=", token_type::GE, ">="},
    {">", token_type::GT, ">"},
    {"==", token_type::EQ, "=="},
    {"=", token_type::ASSIGN, "="},
    {"[a-zA-Z_][a-zA-Z0-9_]*", token_type::ID, "ID"},
    {"[0-9]+\\.[0-9]*", token_type::REALNUM, "REALNUM"},
    {"[0-9]+", token_type::INTNUM, "INTNUM"},
    {"[ \t\n]+", token_type::WHITESPACE, "WHITESPACE"}};

class lexer_tests : public ::testing::Test {
protected:
    lexer::lexer lex;

    lexer_tests()
        : lex(keywords, token_type::WHITESPACE) {}

    void expect_tokens(const std::string& input, const std::vector<std::pair<token_type, std::string>>& expected) const {
        const auto tokens = lex.parse(input);
        ASSERT_EQ(tokens.size(), expected.size());

        for (std::size_t i = 0; i < expected.size(); ++i) {
            EXPECT_EQ(tokens[i].type, static_cast<int>(expected[i].first)) << " at index " << i;
            EXPECT_EQ(tokens[i].value, expected[i].second) << " at index " << i;
        }
    }
};

TEST_F(lexer_tests, recognizes_keywords) {
    expect_tokens("int real if then else", {{token_type::INT, "int"}, {token_type::REAL, "real"}, {token_type::IF, "if"}, {token_type::THEN, "then"}, {token_type::ELSE, "else"}});
}

TEST_F(lexer_tests, recognizes_symbols_and_operators) {
    expect_tokens("(){};+-*/", {{token_type::LPAR, "("}, {token_type::RPAR, ")"}, {token_type::LBRACE, "{"}, {token_type::RBRACE, "}"}, {token_type::SEMI, ";"}, {token_type::PLUS, "+"}, {token_type::MINUS, "-"}, {token_type::MULT, "*"}, {token_type::DIV, "/"}});
}

TEST_F(lexer_tests, recognizes_comparison_operators) {
    expect_tokens("< <= > >= == =", {{token_type::LT, "<"}, {token_type::LE, "<="}, {token_type::GT, ">"}, {token_type::GE, ">="}, {token_type::EQ, "=="}, {token_type::ASSIGN, "="}});
}

TEST_F(lexer_tests, recognizes_identifier_and_numbers) {
    expect_tokens("x var_name INT123 42 3.14", {{token_type::ID, "x"}, {token_type::ID, "var_name"}, {token_type::ID, "INT123"}, {token_type::INTNUM, "42"}, {token_type::REALNUM, "3.14"}});
}

TEST_F(lexer_tests, skips_whitespace) {
    expect_tokens("   int\t\tif\nelse  ", {{token_type::INT, "int"}, {token_type::IF, "if"}, {token_type::ELSE, "else"}});
}

TEST_F(lexer_tests, complex_expression) {
    expect_tokens("if (x <= 42) { y = y + 1; }", {{token_type::IF, "if"}, {token_type::LPAR, "("}, {token_type::ID, "x"}, {token_type::LE, "<="}, {token_type::INTNUM, "42"}, {token_type::RPAR, ")"}, {token_type::LBRACE, "{"}, {token_type::ID, "y"}, {token_type::ASSIGN, "="}, {token_type::ID, "y"}, {token_type::PLUS, "+"}, {token_type::INTNUM, "1"}, {token_type::SEMI, ";"}, {token_type::RBRACE, "}"}});
}

TEST_F(lexer_tests, identifiers_with_underscores_and_digits) {
    expect_tokens("_x x1 _abc123", {{token_type::ID, "_x"}, {token_type::ID, "x1"}, {token_type::ID, "_abc123"}});
}

TEST_F(lexer_tests, leading_zero_integer_and_real) {
    expect_tokens("007 0.5 000.123", {{token_type::INTNUM, "007"}, {token_type::REALNUM, "0.5"}, {token_type::REALNUM, "000.123"}});
}

TEST_F(lexer_tests, real_number_without_fraction_is_accepted) {
    expect_tokens("123.", {{token_type::REALNUM, "123."}});
}

TEST_F(lexer_tests, real_number_without_integer_is_rejected) {
    expect_tokens(".123", {{static_cast<token_type>(-1), "."}, {token_type::INTNUM, "123"}});
}

TEST_F(lexer_tests, compound_expression_with_mixed_tokens) {
    expect_tokens("real result = 3.14 + x1 * (y2 - 42);", {{token_type::REAL, "real"}, {token_type::ID, "result"}, {token_type::ASSIGN, "="}, {token_type::REALNUM, "3.14"}, {token_type::PLUS, "+"}, {token_type::ID, "x1"}, {token_type::MULT, "*"}, {token_type::LPAR, "("}, {token_type::ID, "y2"}, {token_type::MINUS, "-"}, {token_type::INTNUM, "42"}, {token_type::RPAR, ")"}, {token_type::SEMI, ";"}});
}

TEST_F(lexer_tests, empty_input_returns_no_tokens) {
    expect_tokens("", {});
}

TEST_F(lexer_tests, input_with_only_whitespace) {
    expect_tokens("   \t\n  ", {});
}

TEST_F(lexer_tests, handles_long_mixed_input) {
    expect_tokens("int x = 1; if (x >= 10) { x = x + 1; } else { x = 0; }", {{token_type::INT, "int"}, {token_type::ID, "x"}, {token_type::ASSIGN, "="}, {token_type::INTNUM, "1"}, {token_type::SEMI, ";"}, {token_type::IF, "if"}, {token_type::LPAR, "("}, {token_type::ID, "x"}, {token_type::GE, ">="}, {token_type::INTNUM, "10"}, {token_type::RPAR, ")"}, {token_type::LBRACE, "{"}, {token_type::ID, "x"}, {token_type::ASSIGN, "="}, {token_type::ID, "x"}, {token_type::PLUS, "+"}, {token_type::INTNUM, "1"}, {token_type::SEMI, ";"}, {token_type::RBRACE, "}"}, {token_type::ELSE, "else"}, {token_type::LBRACE, "{"}, {token_type::ID, "x"}, {token_type::ASSIGN, "="}, {token_type::INTNUM, "0"}, {token_type::SEMI, ";"}, {token_type::RBRACE, "}"}});
}

TEST_F(lexer_tests, handles_error_at_end_of_input) {
    expect_tokens("int i = 1; i = .",{{token_type::INT, "int"}, {token_type::ID, "i"}, {token_type::ASSIGN, "="}, {token_type::INTNUM, "1"}, {token_type::SEMI, ";"}, {token_type::ID, "i"}, {token_type::ASSIGN, "="}, {static_cast<token_type>(-1), "."}});
}
