#include "grammar/LL1.hpp"
#include "grammar/production.hpp"
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

std::vector<grammar::production::symbol> simple_lexer(const std::string& str) {
    std::vector<grammar::production::symbol> tokens;

    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        tokens.push_back(grammar::production::symbol(token));
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
}