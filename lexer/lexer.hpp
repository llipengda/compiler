#pragma once
#ifndef LEXER_LEXER_HPP
#define LEXER_LEXER_HPP

#include "../regex/regex.hpp"
#include <algorithm>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace lexer {
struct token {
    int type;
    std::string value;
    std::size_t line;
    std::size_t column;

    template <typename TokenType>
    token(const TokenType type, std::string value, const std::size_t line, const std::size_t column)
        : type(static_cast<int>(type)), value(std::move(value)), line(line), column(column) {
        static_assert(std::is_enum<TokenType>::value || std::is_convertible_v<TokenType, int>, "token_type must be an enum type");
    }

    explicit token(std::string value) : type(-1), value(std::move(value)), line(0), column(0) {}

    explicit operator std::string() const;
};

class lexer {
public:
    using tokens_t = std::vector<token>;
    using keyword_t = std::pair<regex::regex, int>;

    template <typename TokenType>
    using input_keywords_t = std::vector<std::tuple<regex::regex, TokenType, std::string>>;

    static int whitespace;
    static std::unordered_map<int, std::string> token_names;

    template <typename TokenType>
    lexer(const input_keywords_t<TokenType> key_words, TokenType whitespace_) {
        static_assert(std::is_enum<TokenType>::value, "token_type must be an enum type");
        whitespace = static_cast<int>(whitespace_);

        for (const auto& [pattern, token, name] : key_words) {
            token_names.insert({static_cast<int>(token), name});
            this->key_words.emplace_back(pattern, static_cast<int>(token));
        }
    }

    tokens_t parse(const std::string& input, bool skip_whitespace = true) const {
        std::size_t max_match = 0;
        std::string cur = input;
        int cur_token;

        std::size_t line = 0;
        std::size_t col = 0;

        tokens_t tokens;

        while (max_match < cur.size()) {
            for (auto& [pattern, token] : key_words) {
                if (const auto match = pattern.match_max(cur); match > max_match) {
                    max_match = match;
                    cur_token = token;
                }
            }

            if (max_match == 0) {
                throw std::runtime_error("No match found at " + cur);
            }

            auto match_str = cur.substr(0, max_match);
            const auto lines = std::count(match_str.begin(), match_str.end(), '\n');
            const auto last_newline = match_str.find_last_of('\n');

            if (!skip_whitespace || cur_token != whitespace) {
                tokens.emplace_back(cur_token, match_str, line + 1, col + 1);
            }

            if (lines > 0) {
                line += lines;
                col = match_str.size() - last_newline - 1;
            } else {
                col += match_str.size();
            }

            cur = cur.substr(max_match);

            max_match = 0;
        }

        return tokens;
    }

private:
    std::vector<keyword_t> key_words;
};

inline std::unordered_map<int, std::string> lexer::token_names{};

inline int lexer::whitespace;

inline token::operator std::string() const {
    if (type == -1) {
        return value;
    }
    return lexer::token_names[type];
}

inline std::ostream& operator<<(std::ostream& os, const token& t) {
    os << "Token(" << lexer::token_names.at(t.type) << ", \"" << t.value << "\", line: " << t.line << ", column: " << t.column << ")";
    return os;
}
} // namespace lexer

#endif // LEXER_LEXER_HPP