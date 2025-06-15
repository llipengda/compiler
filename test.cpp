#define SHOW_DEBUG

#include "grammar/LR1.hpp"
#include "grammar/production.hpp"
#include "lexer/token.hpp"
#include <iostream>
#include <string>
#include <unordered_set>

const std::unordered_set<std::string> terminals = {"+", "*", "a", "b"};

const std::string gram = R"(
E -> E + T | T
T -> T F | F
F -> F * | a | b
)";

const std::string epsilon_str = "e";

std::vector<lexer::token> simple_lexer(const std::string& str) {
    std::vector<lexer::token> tokens;

    std::size_t line = 1;
    std::size_t column = 1;
    std::size_t i = 0;
    while (i < str.size()) {
        if (std::isspace(str[i])) {
            if (str[i] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
            ++i;
            continue;
        }

        std::size_t start_col = column;
        std::size_t start = i;
        while (i < str.size() && !std::isspace(str[i])) {
            ++i;
            ++column;
        }

        std::string lexeme = str.substr(start, i - start);
        tokens.emplace_back(static_cast<lexer::token_type>(0), lexeme, line, start_col);
    }

    return tokens;
}

int main() {
    grammar::production::symbol::set_epsilon_str(epsilon_str);
    grammar::production::symbol::set_terminal_rule([&](const std::string& str) {
        return terminals.count(str);
    });

    std::string str;
    std::string line;
    while (std::getline(std::cin, line)) {
        str += line + '\n';
    }

    grammar::LR1 parser(gram);
    parser.build();
    parser.parse(simple_lexer(str));
    parser.print_steps();
}