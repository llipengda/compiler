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

    token err_token(-1, "", -1, -1);
    bool is_err = false;
    while (max_match < cur.size()) {
        for (auto& [pattern, token] : key_words) {
            if (const auto match = pattern.match_max(cur); match > max_match) {
                max_match = match;
                cur_token = token;
            }
        }

        if (max_match == 0) {
            if (is_err) {
                err_token.value += cur[0];
            } else {
                is_err = true;
                err_token = token(-1, std::string{cur[0]}, line + 1, col + 1);
            }
            col++;
            cur = cur.substr(1);
            continue;
        }

        is_err = false;
        if (!err_token.value.empty()) {
            tokens.emplace_back(std::move(err_token));
            err_token = token(-1, "", -1, -1);
        }

        auto match_str = cur.substr(0, max_match);
        const auto lines = std::ranges::count(match_str, '\n');
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

    if (is_err) {
        tokens.emplace_back(std::move(err_token));
    }

    return tokens;
}

std::unordered_map<int, std::string> lexer::token_names{};
int lexer::whitespace;

token::operator std::string() const {
    if (type == -1) {
        return value;
    }
    return lexer::token_names[type];
}

std::ostream& operator<<(std::ostream& os, const token& t) {
    const std::string type = lexer::token_names.contains(t.type) ? lexer::token_names[t.type] : std::to_string(t.type);
    os << "Token(" << type << ", \"" << t.value << "\", line: " << t.line << ", column: " << t.column << ")";
    return os;
}

} // namespace lexer
