// #define SHOW_DEBUG
#include "lexer/lexer.hpp"

int main() {
    auto lex = lexer::lexer();
    std::string input;

    std::string tmp;
    while (std::getline(std::cin, tmp)) {
        input += tmp + '\n';
    }

    auto res = lex.parse(input);

    for (int i = 0; i < res.size(); ++i) {
        std::cout << i + 1 << ": <" << res[i].first << ',' << res[i].second << ">\n";
    }
}