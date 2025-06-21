#include "lexer/lexer.hpp"
#include <algorithm>

#ifdef USE_STD_REGEX

namespace lexer {

regex_wrapper::regex_wrapper(const std::string& pattern)
    : regex_(pattern) {}

std::size_t regex_wrapper::match_max(const std::string& input) const {
    std::smatch match;
    if (std::regex_search(input, match, regex_, std::regex_constants::match_continuous)) {
        return match.length();
    }
    return 0;
}

} // namespace lexer

#endif

namespace lexer {

lexer::tokens_t lexer::parse(const std::string& input, bool skip_whitespace) const {
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

} // namespace lexer
