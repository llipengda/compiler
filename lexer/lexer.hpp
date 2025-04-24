#pragma once
#ifndef LEXER_LEXER_HPP
#define LEXER_LEXER_HPP

#include "key_words.hpp"
#include <string>
#include <utility>
#include <vector>

namespace lexer {
class lexer {
public:
    lexer() = default;

    using tokens_t = std::vector<std::pair<std::string, token::token_type>>;

    tokens_t parse(const std::string& input) {
        std::size_t max_match = 0;
        std::string cur = input;
        token::token_type cur_token;

        tokens_t tokens;

        while (max_match < cur.size()) {
            for (auto& [pattern, token] : token::key_words) {
                auto match = pattern.match_max(cur);

                if (match > max_match) {
                    max_match = match;
                    cur_token = token;
                }
            }

            if (max_match == 0) {
                throw std::runtime_error("No match found at " + cur);
            }

            if (cur_token != token::token_type::WHITESPACE) {
                tokens.emplace_back(cur.substr(0, max_match), cur_token);
            }

            cur = cur.substr(max_match);

            max_match = 0;
        }
        return tokens;
    }
};
} // namespace lexer

#endif // LEXER_LEXER_HPP