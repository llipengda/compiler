#include "grammar/LL1.hpp"
#include "grammar/production.hpp"
#include "lexer/token.hpp"
#include <iostream>
#include <string>
#include <unordered_set>

const std::unordered_set<std::string> terminals = {
    "{", "}", "(", ")", "if", "then", "else", "while", "=", ";", "<", ">", "<=", ">=", "==", "+", "-", "*", "/", "ID", "NUM"};

const std::string gram = R"(
program -> compoundstmt
stmt ->  ifstmt  |  whilestmt  |  assgstmt  |  compoundstmt
compoundstmt ->  { stmts }
stmts ->  stmt stmts   |   E
ifstmt ->  if ( boolexpr ) then stmt else stmt
whilestmt ->  while ( boolexpr ) stmt
assgstmt ->  ID = arithexpr ;
boolexpr  ->  arithexpr boolop arithexpr
boolop ->   <  |  >  |  <=  |  >=  | ==
arithexpr  ->  multexpr arithexprprime
arithexprprime ->  + multexpr arithexprprime  |  - multexpr arithexprprime  |   E
multexpr ->  simpleexpr  multexprprime
multexprprime ->  * simpleexpr multexprprime  |  / simpleexpr multexprprime  |   E
simpleexpr ->  ID  |  NUM  |  ( arithexpr )
)";

std::vector<lexer::token::token> simple_lexer(const std::string& str) {
    std::vector<lexer::token::token> tokens;

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
        tokens.push_back(lexer::token::token{(lexer::token::token_type)0, lexeme, line, start_col});
    }

    return tokens;
}

int main() {
    grammar::production::symbol::set_epsilon_str("E");
    grammar::production::symbol::set_terminal_rule([&](const std::string& str) {
        return terminals.count(str);
    });

    std::string str;
    std::string line;
    while (std::getline(std::cin, line)) {
        str += line + '\n';
    }

    grammar::LL1 ll1(gram);
    ll1.parse(simple_lexer(str));
    ll1.print_tree();
}