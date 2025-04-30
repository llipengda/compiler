#pragma once
#ifndef LEXER_LEXER_HPP
#define LEXER_LEXER_HPP

#include "keywords.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace lexer {
class lexer {
public:
    lexer() = default;

    using tokens_t = std::vector<token::token>;

    tokens_t parse(const std::string& input) {
        std::size_t max_match = 0;
        std::string cur = input;
        token::token_type cur_token;

        std::size_t line = 0;
        std::size_t col = 0;

        tokens_t tokens;

        while (max_match < cur.size()) {
            for (auto& [pattern, token] : key_words) {
                auto match = pattern.match_max(cur);

                if (match > max_match) {
                    max_match = match;
                    cur_token = token;
                }
            }

            if (max_match == 0) {
                throw std::runtime_error("No match found at " + cur);
            }

            auto match_str = cur.substr(0, max_match);
            auto lines = std::count(match_str.begin(), match_str.end(), '\n');
            auto last_newline = match_str.find_last_of('\n');

            if (cur_token != token::token_type::WHITESPACE) {
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
};
} // namespace lexer

#endif // LEXER_LEXER_HPP