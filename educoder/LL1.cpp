// C语言词法分析器
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
/* 不要修改这个标准输入函数 */
void read_prog(string& prog)
{
	char c;
	while(scanf("%c",&c)!=EOF){
		prog += c;
	}
}
/* 你可以添加其他函数 */

#include <iostream>
#include <string>

#ifndef LEXER_TOKEN_HPP
#define LEXER_TOKEN_HPP

#include <string>

namespace lexer::token {
enum token_type {
    AUTO = 1,
    BREAK,
    CASE,
    CHAR,
    CONST,
    CONTINUE,
    DEFAULT,
    DO,
    DOUBLE,
    ELSE,
    ENUM,
    EXTERN,
    FLOAT,
    FOR,
    GOTO,
    IF,
    INT,
    LONG,
    REGISTER,
    RETURN,
    SHORT,
    SIGNED,
    SIZEOF,
    STATIC,
    STRUCT,
    SWITCH,
    TYPEDEF,
    UNION,
    UNSIGNED,
    VOID,
    VOLATILE,
    WHILE,
    MINUS,
    MINUS_MINUS,
    MINUS_EQUAL,
    ARROW,
    BANG,
    BANG_EQUAL,
    PERCENT,
    PERCENT_EQUAL,
    AMPERSAND,
    AMPERSAND_AMPERSAND,
    AMPERSAND_EQUAL,
    LPAREN,
    RPAREN,
    ASTERISK,
    ASTERISK_EQUAL,
    COMMA,
    DOT,
    SLASH,
    SLASH_EQUAL,
    COLON,
    SEMICOLON,
    QUESTION,
    LBRACKET,
    RBRACKET,
    CARET,
    CARET_EQUAL,
    LBRACE,
    PIPE,
    PIPE_PIPE,
    PIPE_EQUAL,
    RBRACE,
    TILDE,
    PLUS,
    PLUS_PLUS,
    PLUS_EQUAL,
    LESS,
    LESS_LESS,
    LESS_LESS_EQUAL,
    LESS_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    GREATER_GREATER,
    GREATER_GREATER_EQUAL,
    QUOTE,
    COMMENT,
    NUMBER,
    IDENTIFIER,
    WHITESPACE
};

struct token {
    token_type type;
    std::string value;
    std::size_t line;
    std::size_t column;

    token(token_type type, const std::string& value, std::size_t line, std::size_t column)
        : type(type), value(value), line(line), column(column) {}

    explicit token(const std::string& value) : type((token_type)0), value(value), line(0), column(0) {}

    operator std::string() const {
        return value;
    }
};
} // namespace lexer::token

#endif // LEXER_TOKEN_HPP

#ifndef GRAMMAR_PRODUCTION_HPP
#define GRAMMAR_PRODUCTION_HPP

#include <cctype>
#include <functional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace grammar::production {
struct symbol {
    enum class type {
        terminal,
        non_terminal,
        epsilon,
        end_mark
    };

    type type;
    std::string name;

    static std::string epsilon_str;
    static std::string end_mark_str;

    static symbol epsilon;
    static symbol end_mark;

    static std::function<bool(const std::string&)> terminal_rule;

    symbol() {
        type = type::epsilon;
        name = std::string(epsilon_str);
    }

    explicit symbol(const std::string& str) {
        auto trimed = trim(str);

        if (str == epsilon_str) {
            type = type::epsilon;
        } else if (trimed == end_mark_str) {
            type = type::end_mark;
        } else if (terminal_rule(trimed)) {
            type = type::terminal;
        } else {
            type = type::non_terminal;
        }

        name = trimed;
    }

    bool is_terminal() const {
        return type == type::terminal || type == type::epsilon;
    }

    bool is_non_terminal() const {
        return type == type::non_terminal;
    }

    bool is_epsilon() const {
        return type == type::epsilon;
    }

    bool is_end_mark() const {
        return type == type::end_mark;
    }

    bool operator==(const symbol& other) const {
        return type == other.type && name == other.name;
    }

    static void set_epsilon_str(const std::string& str) {
        epsilon_str = str;
        epsilon = symbol(epsilon_str);
    }

    static void set_end_mark_str(const std::string& str) {
        end_mark_str = str;
        end_mark = symbol(end_mark_str);
    }

    static void set_terminal_rule(const std::function<bool(const std::string&)>& rule) {
        terminal_rule = rule;
    }

    static void set_terminal_rule(std::function<bool(const std::string&)>&& rule) {
        terminal_rule = std::move(rule);
    }

private:
    static std::string trim(const std::string& str) {
        std::size_t start = str.find_first_not_of(" \t\n\r");
        std::size_t end = str.find_last_not_of(" \t\n\r");
        if (start == std::string::npos || end == std::string::npos) {
            return "";
        } else {
            return str.substr(start, end - start + 1);
        }
    }
};

std::string symbol::epsilon_str = "ε";
std::string symbol::end_mark_str = "$";

std::function<bool(const std::string&)> symbol::terminal_rule = [](const std::string& str) {
    return !std::isupper(str[0]);
};

symbol symbol::epsilon = symbol(epsilon_str);
symbol symbol::end_mark = symbol(end_mark_str);

std::ostream& operator<<(std::ostream& os, const symbol& sym) {
    os << sym.name;
    return os;
}

class production {
public:
    symbol lhs;
    std::vector<symbol> rhs;

    production() = default;

    explicit production(const std::string& str) {
        std::size_t pos = str.find("->");
        if (pos == std::string::npos) {
            throw std::invalid_argument("Invalid production format: " + str);
        }

        lhs = symbol(str.substr(0, pos));
        std::string rhs_str = str.substr(pos + 2);

        std::size_t start = 0;
        std::size_t end = 0;
        while ((end = rhs_str.find(' ', start)) != std::string::npos) {
            auto s = rhs_str.substr(start, end - start);
            if (s.empty()) {
                start = end + 1;
                continue;
            }
            rhs.push_back(symbol(s));
            start = end + 1;
        }
        auto remain = rhs_str.substr(start);
        if (!remain.empty()) {
            rhs.push_back(symbol(remain));
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const production& prod);

    static const std::vector<production> parse(const std::string& str) {
        std::vector<production> productions;
        std::size_t pos = 0;
        while (pos < str.size()) {
            auto end_pos = str.find('\n', pos);
            if (end_pos == std::string::npos) {
                end_pos = str.size();
            }
            auto prod_str = str.substr(pos, end_pos - pos);
            // process | to multiple productions
            auto pipe_pos = prod_str.find('|');
            std::string lhs_str = "";
            while (pipe_pos != std::string::npos) {
                productions.emplace_back(lhs_str + prod_str.substr(0, pipe_pos));
                if (lhs_str.empty()) {
                    lhs_str = productions.rbegin()->lhs.name + " -> ";
                }
                prod_str = prod_str.substr(pipe_pos + 1);
                pipe_pos = prod_str.find('|');
            }
            if (!prod_str.empty()) {
                productions.emplace_back(lhs_str + prod_str);
            }
            pos = end_pos + 1;
        }
        return productions;
    }

    std::string to_string() const {
        std::string result = lhs.name + " -> ";
        for (const auto& sym : rhs) {
            result += sym.name + " ";
        }
        return result;
    }
};

std::ostream& operator<<(std::ostream& os, const production& prod) {
    os << prod.to_string();
    return os;
}
} // namespace grammar::production

namespace std {
template <>
struct hash<grammar::production::symbol> {
    std::size_t operator()(const grammar::production::symbol& sym) const {
        return std::hash<std::string>()(sym.name);
    }
};
} // namespace std

#endif // GRAMMAR_PRODUCTION_HPP

#ifndef GRAMMAR_EXCEPTION_HPP
#define GRAMMAR_EXCEPTION_HPP

#include <exception>
#include <string>

namespace grammar::exception {
class ambiguous_grammar_exception : public std::exception {
public:
    ambiguous_grammar_exception(const std::vector<grammar::production::production>& prods) {
        for (const auto& prod : prods) {
            msg += prod.to_string() + "\n";
        }
    }

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg = "Ambiguous grammar: \n";
};

class grammar_error : public std::exception {
public:
    grammar_error(const std::string& message) : msg(message) {}

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};
} // namespace grammar::exception

#endif // GRAMMAR_EXCEPTION_HPP


#ifndef GRAMMAR_TREE_HPP
#define GRAMMAR_TREE_HPP

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

namespace grammar {
struct tree_node {
    production::symbol symbol;
    std::vector<std::shared_ptr<tree_node>> children;
    std::shared_ptr<tree_node> parent = nullptr;
};

class tree {
    std::shared_ptr<tree_node> root = nullptr;
    std::shared_ptr<tree_node> next = nullptr;

public:
    void add(const production::production& prod) {
        if (!root) {
            root = std::make_shared<tree_node>();
            root->symbol = prod.lhs;
            for (const auto& rhs : prod.rhs) {
                auto node = std::make_shared<tree_node>();
                node->symbol = rhs;
                node->parent = root;
                root->children.push_back(node);
                if (!next && rhs.is_non_terminal()) {
                    next = node;
                }
            }
            return;
        }
        assert(next != nullptr);
        assert(next->children.empty());
        assert(next->symbol == prod.lhs);
        bool found = false;
        auto new_next = std::make_shared<tree_node>();
        for (const auto& rhs : prod.rhs) {
            auto node = std::make_shared<tree_node>();
            node->symbol = rhs;
            node->parent = next;
            next->children.push_back(node);
            if (!found && rhs.is_non_terminal()) {
                new_next = node;
                found = true;
            }
        }
        if (found) {
            next = new_next;
        } else {
            next = next->parent;
            while (next) {
                for (const auto& child : next->children) {
                    if (child->symbol.is_non_terminal() && child->children.empty()) {
                        next = child;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
                next = next->parent;
            }
        }
    }

    void print() const {
        if (root) {
            print_node(root, 0);
        }
    }

    void print_node(const std::shared_ptr<tree_node>& node, int depth) const {
        for (int i = 0; i < depth; ++i) {
            std::cout << '\t';
        }
        std::cout << node->symbol << "\n";
        for (const auto& child : node->children) {
            print_node(child, depth + 1);
        }
    }
};
} // namespace grammar

#endif // GRAMMAR_TREE_HPP

#ifndef GRAMMAR_LL1_HPP
#define GRAMMAR_LL1_HPP

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace grammar {
class grammar {
    virtual void parse(const std::vector<lexer::token::token>&) = 0;
};

class LL1 : public grammar {
public:
    using symbol_set = std::unordered_set<production::symbol>;
    using table_t = std::unordered_map<production::symbol, std::unordered_map<production::symbol, production::production>>;

    LL1(const std::vector<production::production>& productions) : productions(productions) {
        for (std::size_t i = 0; i < productions.size(); ++i) {
            const auto& prod = productions[i];
            symbol_map[prod.lhs].push_back(i);
        }

        build();
    }

    LL1(const std::string& str) {
        productions = production::production::parse(str);
        for (std::size_t i = 0; i < productions.size(); ++i) {
            const auto& prod = productions[i];
            symbol_map[prod.lhs].push_back(i);
        }

        build();
    }

    LL1(const LL1&) = delete;
    LL1& operator=(const LL1&) = delete;
    LL1(LL1&&) = default;
    LL1& operator=(LL1&&) = default;

    void parse(const std::vector<lexer::token::token>& input) override {
        // std::vector<production::production> output;

        auto in = input;
        in.emplace_back(production::symbol::end_mark_str);

        std::stack<production::symbol> stack;
        stack.push(production::symbol::end_mark);
        stack.push(productions[0].lhs);

        std::size_t pos = 0;

        while (pos < in.size() || !stack.empty()) {
            auto cur_input = production::symbol{in[pos]};
            auto& top = stack.top();

            auto get_pos = [&]() {
                return "at line " + std::to_string(in[pos].line) + ", column " + std::to_string(in[pos].column);
            };

            if (top.is_terminal() || top.is_end_mark()) {
                if (top == cur_input) {
                    pos++;
                    stack.pop();
                } else {
                    stack.pop();
                    std::cout << "语法错误,第" << in[pos].line - 1 << "行," << "缺少\"" << top.name << "\"\n";
                    // std::cerr << "Expect: " << top.name << " but got: " << cur_input.name << ' ' << get_pos() << '\n';
                    // throw exception::grammar_error("Unexpected token: expected " + top.name + " instead of " + cur_input.name);
                }
            } else {
                const auto& table = parsing_table.at(top);
                if (!table.count(cur_input)) {
                    // throw exception::grammar_error("Unexpected token: " + cur_input.name);
                    if (first.at(top).count(production::symbol::epsilon)) {
                        stack.pop();
                        tree_.add(production::production(top.name + " -> " + production::symbol::epsilon_str));
                    } else if (!follow.at(top).count(cur_input)) {
                        pos++;
                    } else {
                        throw exception::grammar_error("Unexpected token: " + cur_input.name + ' ' + get_pos());
                    }
                    // std::cerr << "Unexpected token: " << cur_input.name << ' ' << get_pos() << '\n';
                    continue;
                }
                stack.pop();
                const auto& prod = table.at(cur_input);
                // output.push_back(prod);
                tree_.add(prod);
                const auto& symbols = prod.rhs;

                if (symbols.size() == 1 && symbols[0].is_epsilon()) {
                    continue;
                }

                for (auto it = symbols.rbegin(); it != symbols.rend(); it++) {
                    stack.push(*it);
                }
            }
        }

        // for (const auto& prod : output) {
        //     std::cout << prod << '\n';
        // }
    }

    void print_first() const {
        std::cout << "FIRST sets:\n";
        for (const auto& p : first) {
            auto& sym = p.first;
            auto& first_set = p.second;
            if (sym.is_terminal()) {
                continue;
            }
            std::cout << "FIRST(" << sym << ") = {";
            for (auto it = first_set.begin(); it != first_set.end(); ++it) {
                std::cout << *it;
                if (std::next(it) != first_set.end()) {
                    std::cout << ',';
                }
            }
            std::cout << "}\n";
        }
        std::cout << '\n';
    }

    void print_follow() const {
        std::cout << "FOLLOW sets:\n";
        for (const auto& p : follow) {
            auto& sym = p.first;
            auto& follow_set = p.second;
            std::cout << "FOLLOW(" << sym << ") = {";
            for (auto it = follow_set.begin(); it != follow_set.end(); ++it) {
                std::cout << *it;
                if (std::next(it) != follow_set.end()) {
                    std::cout << ',';
                }
            }
            std::cout << "}\n";
        }
        std::cout << '\n';
    }

    void print_productions() const {
        std::cout << "Productions:\n";
        for (const auto& prod : productions) {
            std::cout << prod << '\n';
        }
        std::cout << '\n';
    }

    void print_parsing_table() const {
        std::cout << "Parsing Table:\n";
        for (const auto& p : parsing_table) {
            auto& nt = p.first;
            auto& table = p.second;
            for (const auto& pp : table) {
                auto& t = pp.first;
                auto& prod = pp.second;
                std::cout << "M[" << nt << "," << t << "] = " << prod << '\n';
            }
        }
    }

    void print_parsing_table_pretty() const {
        auto longest_prod_len =
            std::max_element(
                productions.begin(),
                productions.end(),
                [](const production::production& a, const production::production& b) {
                    return a.to_string().size() < b.to_string().size();
                })
                ->to_string()
                .size();
        using t = decltype(*follow.begin());
        auto longest_non_terminal_sym_len =
            std::max_element(
                follow.begin(),
                follow.end(),
                [](t a, t b) {
                    return a.first.name.size() < b.first.name.size();
                })
                ->first.name.size();
        symbol_set terminals;
        symbol_set non_terminals;
        for (const auto& p : first) {
            auto& sym = p.first;
            if ((sym.is_terminal() || sym.is_end_mark()) && !sym.is_epsilon()) {
                terminals.insert(sym);
            }
            if (sym.is_non_terminal()) {
                non_terminals.insert(sym);
            }
        }
        auto width = longest_non_terminal_sym_len + longest_prod_len * terminals.size() + (terminals.size() + 2);
        auto line = std::string(width, '-');
        std::cout << "Parsing Table:\n"
                  << line << '\n';
        std::cout << '|' << std::left << std::setw(longest_non_terminal_sym_len) << " " << '|';
        for (const auto& t : terminals) {
            std::cout << std::left << std::setw(longest_prod_len) << t << '|';
        }
        std::cout << '\n'
                  << line << '\n';
        for (const auto& nt : non_terminals) {
            std::cout << '|' << std::left << std::setw(longest_non_terminal_sym_len) << nt << '|';
            for (const auto& t : terminals) {
                if (parsing_table.at(nt).count(t)) {
                    const auto& prod = parsing_table.at(nt).at(t);
                    std::cout << std::left << std::setw(longest_prod_len) << prod << '|';
                } else {
                    std::cout << std::left << std::setw(longest_prod_len) << " " << '|';
                }
            }
            std::cout << '\n'
                      << line << '\n';
        }
    }

    void print_tree() const {
        tree_.print();
    }

private:
    std::vector<production::production> productions;
    std::unordered_map<production::symbol, symbol_set> first;
    std::unordered_map<production::symbol, symbol_set> follow;

    std::unordered_map<production::symbol, std::vector<std::size_t>> symbol_map;
    table_t parsing_table;

    tree tree_;

    void build() {
        calc_first();
        calc_follow();
        build_parsing_table();
    }

    void calc_first() {
        for (const auto& prod : productions) {
            calc_first(prod.lhs);
        }
    }

    symbol_set& calc_first(production::symbol sym) {
        if (first.count(sym)) {
            return first[sym];
        }

        if (sym.is_terminal() || sym.is_epsilon()) {
            first[sym] = {sym};
            return first[sym];
        }

        auto& production_ids = symbol_map[sym];
        auto& result = first[sym];

        for (const auto& id : production_ids) {
            const auto& prod = productions[id];
            const auto& rhs = prod.rhs;

            for (const auto& s : rhs) {
                auto first_set = calc_first(s);
                result.insert(first_set.begin(), first_set.end());

                if (!first_set.count(production::symbol::epsilon)) {
                    break;
                }
            }
        }

        return result;
    }

    void calc_follow() {
        follow[productions[0].lhs].insert(production::symbol::end_mark);
        while (true) {
            bool changed = false;
            for (std::size_t i = 0; i < productions.size(); ++i) {
                changed |= calc_follow(i);
            }
            if (!changed) {
                break;
            }
        }

        for (auto& p: follow) {
            auto& follow_set = p.second;
            follow_set.erase(production::symbol::epsilon);
        }
    }

    bool calc_follow(const std::size_t pos) {
        auto& prod = productions[pos];
        auto& rhs = prod.rhs;
        auto& lhs = prod.lhs;

        const auto follow_copy = follow;

        for (std::size_t i = 0; i < rhs.size(); ++i) {
            auto& sym = rhs[i];
            if (!sym.is_non_terminal()) {
                continue;
            }
            if (i + 1 < rhs.size()) {
                auto& next_sym = rhs[i + 1];
                auto& first_set = calc_first(next_sym);
                follow[sym].insert(first_set.begin(), first_set.end());
            }
        }

        auto last = rhs.rbegin();
        if (last != rhs.rend()) {
            auto& sym = *last;
            if (sym.is_non_terminal()) {
                follow[sym].insert(follow[lhs].begin(), follow[lhs].end());
            }
        }

        for (auto it = rhs.rbegin(); it != rhs.rend(); ++it) {
            auto& sym = *it;
            if (std::next(it) == rhs.rend()) {
                continue;
            }
            auto& prev_sym = *std::next(it);

            if (!prev_sym.is_non_terminal()) {
                continue;
            }
            auto& first_set = calc_first(sym);
            if (first_set.count(production::symbol::epsilon)) {
                follow[prev_sym].insert(follow[lhs].begin(), follow[lhs].end());
            } else {
                break;
            }
        }

        return follow != follow_copy;
    }

    symbol_set calc_first(const production::production& prod) const {
        symbol_set result;
        for (const auto& sym : prod.rhs) {
            auto first_set = first.at(sym);
            result.insert(first_set.begin(), first_set.end());
            if (!first_set.count(production::symbol::epsilon)) {
                break;
            }
        }
        return result;
    }

    void build_parsing_table() {
        for (const auto& prod : productions) {
            auto first_set = calc_first(prod);
            for (const auto& sym : first_set) {
                if (sym.is_terminal() && !sym.is_epsilon()) {
                    if (parsing_table[prod.lhs].count(sym)) {
                        throw exception::ambiguous_grammar_exception({prod, parsing_table[prod.lhs][sym]});
                    }
                    parsing_table[prod.lhs][sym] = prod;
                }
            }
            if (first_set.count(production::symbol::epsilon)) {
                auto follow_set = follow[prod.lhs];
                for (const auto& sym : follow_set) {
                    if (sym.is_terminal() || sym.is_end_mark()) {
                        if (parsing_table[prod.lhs].count(sym)) {
                            throw exception::ambiguous_grammar_exception({prod, parsing_table[prod.lhs][sym]});
                        }
                        parsing_table[prod.lhs][sym] = prod;
                    }
                }
            }
        }
    }
};
} // namespace grammar

#endif // GRAMMAR_LL1_HPP

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

void Analysis()
{
	string prog;
	read_prog(prog);
	/* 骚年们 请开始你们的表演 */
    /********* Begin *********/
    grammar::production::symbol::set_epsilon_str("E");
    grammar::production::symbol::set_terminal_rule([&](const std::string& str) {
        return terminals.count(str);
    });
    grammar::LL1 ll1(gram);
    ll1.parse(simple_lexer(prog));
    ll1.print_tree();
    /********* End *********/
	
}

int main() {
    Analysis();
}