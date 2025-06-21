#pragma once
#ifndef LEXER_LEXER_HPP
#define LEXER_LEXER_HPP

#ifdef USE_STD_REGEX
#include <regex>
#else
#include "regex/regex.hpp"
#endif

#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


namespace lexer {

#ifdef USE_STD_REGEX
class regex_wrapper {
public:
    explicit regex_wrapper(const std::string& pattern);
    std::size_t match_max(const std::string& input) const;

private:
    std::regex regex_;
};
#else
using regex_wrapper = regex::regex;
#endif

struct token {
    int type;
    std::string value;
    std::size_t line;
    std::size_t column;

    template <typename TokenType>
    token(const TokenType type, std::string value, const std::size_t line, const std::size_t column)
        : type(static_cast<int>(type)), value(std::move(value)), line(line), column(column) {
        static_assert(std::is_enum_v<TokenType> || std::is_convertible_v<TokenType, int>, "token_type must be an enum type");
    }

    explicit token(std::string value) : type(-1), value(std::move(value)), line(0), column(0) {}

    explicit operator std::string() const;

    friend std::ostream& operator<<(std::ostream& os, const token& t);
};

class lexer {
public:
    using tokens_t = std::vector<token>;
    using keyword_t = std::pair<regex_wrapper, int>;

    template <typename TokenType>
    struct input_keyword {
        std::string pattern_str;
        TokenType token;
        std::string name;

        input_keyword(std::string pattern, TokenType tok, std::string n)
            : pattern_str(std::move(pattern)), token(tok), name(std::move(n)) {}
    };

    template <typename TokenType>
    using input_keywords_t = std::vector<input_keyword<TokenType>>;

    static int whitespace;
    static std::unordered_map<int, std::string> token_names;

    template <typename TokenType>
    lexer(input_keywords_t<TokenType> key_words, TokenType whitespace_);

    [[nodiscard]] tokens_t parse(const std::string& input, bool skip_whitespace = true) const;

private:
    std::vector<keyword_t> key_words;
};

} // namespace lexer

#pragma region tpp

namespace lexer {

template <typename TokenType>
lexer::lexer(const input_keywords_t<TokenType> key_words, TokenType whitespace_) {
    static_assert(std::is_enum_v<TokenType>, "token_type must be an enum type");
    whitespace = static_cast<int>(whitespace_);

    for (const auto& keyword : key_words) {
        const auto& pattern_str = keyword.pattern_str;
        const auto& token = keyword.token;
        const auto& name = keyword.name;
        token_names.insert({static_cast<int>(token), name});
        this->key_words.emplace_back(regex_wrapper(pattern_str), static_cast<int>(token));
    }
}

} // namespace lexer

#pragma endregion

#endif // LEXER_LEXER_HPP
