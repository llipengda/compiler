// C语言词法分析器
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
/* 不要修改这个标准输入函数 */
void read_prog(string& prog) {
    char c;
    while (scanf("%c", &c) != EOF) {
        prog += c;
    }
}
/* 你可以添加其他函数 */

#ifndef LEXER_TOKEN_HPP
#define LEXER_TOKEN_HPP

#include <string>
#include <utility>

namespace lexer {
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

    token(const token_type type, std::string value, const std::size_t line, const std::size_t column)
        : type(type), value(std::move(value)), line(line), column(column) {}

    explicit token(std::string value) : type(static_cast<token_type>(0)), value(std::move(value)), line(0), column(0) {}

    explicit operator std::string() const {
        return value;
    }
};
} // namespace lexer

#endif // LEXER_TOKEN_HPP

#ifndef GRAMMAR_PRODUCTION_HPP
#define GRAMMAR_PRODUCTION_HPP

#include <cctype>
#include <cstddef>
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

    [[nodiscard]] bool is_terminal() const {
        return type == type::terminal || type == type::epsilon;
    }

    [[nodiscard]] bool is_non_terminal() const {
        return type == type::non_terminal;
    }

    [[nodiscard]] bool is_epsilon() const {
        return type == type::epsilon;
    }

    [[nodiscard]] bool is_end_mark() const {
        return type == type::end_mark;
    }

    bool operator==(const symbol& other) const {
        return type == other.type && name == other.name;
    }

    bool operator<(const symbol& other) const {
        if (type != other.type) {
            return static_cast<int>(type) < static_cast<int>(other.type);
        }
        return name < other.name;
    }

    bool operator!=(const symbol& other) const {
        return !(*this == other);
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

inline std::ostream& operator<<(std::ostream& os, const symbol& sym) {
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
            rhs.emplace_back(s);
            start = end + 1;
        }
        auto remain = rhs_str.substr(start);
        if (!remain.empty()) {
            rhs.emplace_back(remain);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const production& prod);

    static std::vector<production> parse(const std::string& str) {
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
            std::string lhs_str;
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

    [[nodiscard]] virtual std::string to_string() const {
        std::string result = lhs.name + " -> ";
        for (const auto& sym : rhs) {
            result += sym.name + " ";
        }
        return result;
    }

    virtual bool operator==(const production& other) const {
        return lhs == other.lhs && rhs == other.rhs;
    }

    virtual ~production() = default;
};

inline std::ostream& operator<<(std::ostream& os, const production& prod) {
    os << prod.to_string();
    return os;
}

class LR_production final : public production {
public:
    std::size_t dot_pos{};

    LR_production() = default;

    explicit LR_production(const std::string& str) : production(str) {
        dot_pos = 0;
    }

    explicit LR_production(const production& prod) : production(prod) {
        dot_pos = 0;
        if (rhs.size() == 1 && rhs[0].is_epsilon()) {
            rhs.clear();
        }
    }

    [[nodiscard]] bool is_start() const {
        return dot_pos == 0;
    }

    [[nodiscard]] bool is_end() const {
        return dot_pos == rhs.size();
    }

    [[nodiscard]] bool is_rhs_empty() const {
        return rhs.empty();
    }

    [[nodiscard]] LR_production next() const {
        LR_production next = *this;
        if (dot_pos < rhs.size()) {
            ++next.dot_pos;
        }
        return next;
    }

    [[nodiscard]] const symbol& symbol_after_dot() const {
        if (dot_pos < rhs.size()) {
            return rhs[dot_pos];
        }
        throw std::out_of_range("No symbol after dot");
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result = lhs.name + " -> ";
        for (std::size_t i = 0; i < rhs.size(); ++i) {
            if (i == dot_pos) {
                result += "· ";
            }
            result += rhs[i].name + " ";
        }
        if (dot_pos == rhs.size()) {
            result += "·";
        }
        return result;
    }

    bool operator==(const LR_production& other) const {
        return production::operator==(other) && dot_pos == other.dot_pos;
    }

    bool operator==(const production& other) const override {
        if (lhs != other.lhs) {
            return false;
        }
        if (rhs == other.rhs) {
            return true;
        }
        if (rhs.empty() && other.rhs.size() == 1 && other.rhs[0].is_epsilon()) {
            return true;
        }
        return false;
    }
};
} // namespace grammar::production

namespace std {
template <>
struct hash<grammar::production::symbol> {
    std::size_t operator()(const grammar::production::symbol& sym) const noexcept {
        return std::hash<std::string>()(sym.name);
    }
};

template <>
struct hash<grammar::production::LR_production> {
    std::size_t operator()(const grammar::production::LR_production& lr_prod) const noexcept {
        std::size_t h = std::hash<grammar::production::symbol>()(lr_prod.lhs);
        for (const auto& sym : lr_prod.rhs) {
            h ^= std::hash<grammar::production::symbol>()(sym) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h ^ (lr_prod.dot_pos << 16);
    }
};
} // namespace std

#endif // GRAMMAR_PRODUCTION_HPP

#ifndef GRAMMAR_EXCEPTION_HPP
#define GRAMMAR_EXCEPTION_HPP

#include <exception>
#include <string>

namespace grammar::exception {
class ambiguous_grammar_exception final : public std::exception {
public:
    explicit ambiguous_grammar_exception(const std::vector<grammar::production::production>& prods) {
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

class grammar_error final : public std::exception {
public:
    explicit grammar_error(const std::string& message) : msg(message) {}

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};
} // namespace grammar::exception

#endif // GRAMMAR_EXCEPTION_HPP

#ifndef GRAMMAR_BASE_HPP
#define GRAMMAR_BASE_HPP

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
    std::shared_ptr<tree_node> next_r = nullptr;

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
            for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
                if ((*it)->symbol.is_non_terminal()) {
                    next_r = *it;
                    break;
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

    void add_r(const production::production& prod) {
        if (!root) {
            root = std::make_shared<tree_node>();
            root->symbol = prod.lhs;
            for (const auto& rhs : prod.rhs) {
                auto node = std::make_shared<tree_node>();
                node->symbol = rhs;
                node->parent = root;
                root->children.push_back(node);
            }
            for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
                if ((*it)->symbol.is_non_terminal()) {
                    next_r = *it;
                    break;
                }
            }
            return;
        }

        assert(next_r != nullptr);
        assert(next_r->children.empty());
        assert(next_r->symbol == prod.lhs);

        for (const auto& rhs : prod.rhs) {
            auto node = std::make_shared<tree_node>();
            node->symbol = rhs;
            node->parent = next_r;
            next_r->children.push_back(node);
        }

        bool found = false;

        for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
            if ((*it)->symbol.is_non_terminal()) {
                next_r = *it;
                found = true;
                break;
            }
        }

        if (!found) {
            next_r = next_r->parent;
            while (next_r) {
                for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
                    if ((*it)->symbol.is_non_terminal() && (*it)->children.empty()) {
                        next_r = *it;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
                next_r = next_r->parent;
            }
        }
    }

    void print() const {
        if (root) {
            print_node(root, 0);
        }
    }

    static void print_node(const std::shared_ptr<tree_node>& node, const int depth) {
        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }
        std::cout << node->symbol << "\n";
        for (const auto& child : node->children) {
            print_node(child, depth + 1);
        }
    }
};
} // namespace grammar

#endif // GRAMMAR_TREE_HPP

#include <stack>
#include <unordered_map>
#include <unordered_set>

namespace grammar {
class grammar_base {
public:
    using symbol_set = std::unordered_set<production::symbol>;
    virtual void parse(const std::vector<lexer::token>&) = 0;
    virtual ~grammar_base() = default;

    virtual void print_tree() const {
        tree_.print();
    }

protected:
    std::vector<production::production> productions;
    std::unordered_map<production::symbol, symbol_set> first;
    std::unordered_map<production::symbol, symbol_set> follow;
    std::unordered_map<production::symbol, std::vector<std::size_t>> symbol_map;
    tree tree_;

    void calc_first() {
        for (const auto& prod : productions) {
            calc_first(prod.lhs);
        }
    }

    symbol_set& calc_first(const production::symbol& sym) {
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

        for (auto& follow_it : follow) {
            auto sym = follow_it.first;
            auto follow_set = follow_it.second;
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

    template <typename T>
    static void print_stack(const std::stack<T> stack) {
        std::vector<T> elements(stack.size());
        auto copy = stack;
        auto n = elements.size();
        for (std::size_t i = 0; i < n; ++i) {
            elements[n - i - 1] = copy.top();
            copy.pop();
        }
        for (const auto& elem : elements) {
            std::cout << elem << " ";
        }
        std::cout << std::endl;
    }
};
} // namespace grammar

#endif

#ifndef GRAMMAR_SLR_HPP
#define GRAMMAR_SLR_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace grammar {
struct action {
    enum class type {
        shift,
        reduce,
        accept,
        error
    };

    type action_type;
    std::size_t val;

    [[nodiscard]] bool is_shift() const {
        return action_type == type::shift;
    }

    [[nodiscard]] bool is_reduce() const {
        return action_type == type::reduce;
    }

    [[nodiscard]] bool is_accept() const {
        return action_type == type::accept;
    }

    static action shift(std::size_t val) {
        return {type::shift, val};
    }

    static action reduce(std::size_t val) {
        return {type::reduce, val};
    }

    static action accept() {
        return {type::accept, 0};
    }

    static action error(std::size_t val) {
        return {type::error, val};
    }

    action() : action_type(type::error), val(-1) {}

private:
    action(type action_type, std::size_t val) : action_type(action_type), val(val) {}
};

inline std::ostream& operator<<(std::ostream& os, const action& act) {
    switch (act.action_type) {
    case action::type::shift:
        os << "s" + std::to_string(act.val);
        break;
    case action::type::reduce:
        os << "r" + std::to_string(act.val);
        break;
    case action::type::accept:
        os << "acc";
        break;
    case action::type::error:
        os << "e" + std::to_string(act.val);
        break;
    }
    return os;
}

struct LR_stack_t {
private:
    union {
        std::size_t state;
        production::symbol symbol;
    };

    enum class type {
        state,
        symbol
    } type;

public:
    LR_stack_t() : state(-1), type(type::state) {}
    LR_stack_t(std::size_t s) : state(s), type(type::state) {}
    LR_stack_t(const production::symbol& sym) : symbol(sym), type(type::symbol) {}

    LR_stack_t(const LR_stack_t& other) : type(other.type) {
        if (type == type::state) {
            state = other.state;
        } else {
            new (&symbol) production::symbol(other.symbol);
        }
    }

    LR_stack_t(LR_stack_t&& other) noexcept : type(other.type) {
        if (type == type::state) {
            state = other.state;
        } else {
            new (&symbol) production::symbol(std::move(other.symbol));
        }
    }

    LR_stack_t& operator=(const LR_stack_t& other) {
        if (this != &other) {
            this->~LR_stack_t();
            new (this) LR_stack_t(other);
        }
        return *this;
    }

    LR_stack_t& operator=(LR_stack_t&& other) noexcept {
        if (this != &other) {
            this->~LR_stack_t();
            new (this) LR_stack_t(std::move(other));
        }
        return *this;
    }

    ~LR_stack_t() {
        if (is_symbol()) {
            symbol.~symbol();
        }
    }

    [[nodiscard]] bool is_state() const { return type == type::state; }
    [[nodiscard]] bool is_symbol() const { return type == type::symbol; }

    [[nodiscard]] std::size_t get_state() const {
        assert(is_state());
        return state;
    }

    [[nodiscard]] production::symbol get_symbol() const {
        assert(is_symbol());
        return symbol;
    }
};

inline std::ostream& operator<<(std::ostream& os, const LR_stack_t& stack) {
    if (stack.is_state()) {
        os << stack.get_state();
    } else {
        os << stack.get_symbol();
    }
    return os;
}

struct rightmost_step {
private:
    std::vector<production::symbol> symbols;
    std::vector<std::vector<production::symbol>> steps;

    void add_step() {
        steps.emplace_back(symbols);
    }

public:
    rightmost_step() = default;

    void set_input(const std::vector<lexer::token>& input) {
        for (const auto& token : input) {
            symbols.emplace_back(token.value);
        }
        add_step();
    }

    void add(const production::production& prod, const std::size_t ridx) {
        auto rit = symbols.rbegin() + ridx + prod.rhs.size() - 1;
        if (!(prod.rhs.size() == 1 && prod.rhs[0].is_epsilon())) {
            auto it = rit.base();
            it = symbols.erase(it, it + prod.rhs.size());
            symbols.insert(it, prod.lhs);
        } else {
            for (auto it = symbols.rbegin(); it != symbols.rend(); ++it) {
                if (it->is_non_terminal()) {
                    symbols.insert(it.base(), prod.lhs);
                    break;
                }
            }
        }
        add_step();
    }

    void print() const {
        for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
            for (const auto& sym : *it) {
                std::cout << sym << " ";
            }
            if (std::next(it) != steps.rend()) {
                std::cout << "=> \n";
            }
        }
    }

    void insert_symbol(const std::size_t ridx, const production::symbol& sym) {
        symbols.insert((symbols.rbegin() + ridx - 1).base(), sym);
        for (auto& step : steps) {
            step.insert((step.rbegin() + ridx - 1).base(), sym);
        }
    }
};

class SLR final : public grammar_base {
public:
    using items_t = std::unordered_set<production::LR_production>;
    using after_dot_t = std::unordered_set<production::symbol>;

    explicit SLR(const std::vector<production::production>& productions) {
        this->productions = productions;

        const auto& first_prod = productions[0];
        this->productions.emplace(productions.begin(), first_prod.lhs.name + '\'' + " -> " + first_prod.lhs.name);

        for (std::size_t i = 0; i < productions.size(); ++i) {
            const auto& prod = productions[i];
            symbol_map[prod.lhs].push_back(i);
        }

        build();
    }

    explicit SLR(const std::string& str) {
        productions = production::production::parse(str);

        const auto& first_prod = productions[0];
        productions.emplace(productions.begin(), first_prod.lhs.name + '\'' + " -> " + first_prod.lhs.name);

        for (std::size_t i = 0; i < productions.size(); ++i) {
            const auto& prod = productions[i];
            symbol_map[prod.lhs].push_back(i);
        }

#ifdef SHOW_DEBUG
        std::cout << "Grammar:\n";
        for (const auto& prod : productions) {
            std::cout << prod << std::endl;
        }
#endif

        build();
    }

    void parse(const std::vector<lexer::token>& input) override {
        auto in = input;
        steps.set_input(in);
        in.emplace_back(production::symbol::end_mark_str);

        std::stack<LR_stack_t> stack;
        stack.push(std::size_t{0});

        std::vector<production::production> output;
        std::size_t pos = 0;

        while (pos < in.size() || !stack.empty()) {
            auto cur_input = production::symbol{std::string(in[pos])};
            auto& top = stack.top();

            assert(top.is_state());
            auto row = action_table.at(top.get_state());

            if (!row.count(cur_input)) {
                throw exception::grammar_error("Unexpected token: " + cur_input.name + " at line " + std::to_string(in[pos].line) + ", column " + std::to_string(in[pos].column));
            }
            auto act = row[cur_input];

#ifdef SHOW_DEBUG
            std::cout << "------------------------\n";
            std::cout << "stack: \n";
            print_stack(stack);
            std::cout << "input: ";
            for (std::size_t i = pos; i < in.size(); ++i) {
                std::cout << in[i].value << " ";
            }
            std::cout << "\n";
            std::cout << "action: " << act << '\n';
#endif
            if (act.is_accept()) {
                for (auto it = output.rbegin(); it != output.rend(); ++it) {
                    tree_.add_r(*it);
                }
                return;
            }

            if (act.is_shift()) {
                stack.push(cur_input);
                stack.push(act.val);
                pos++;
            } else if (act.is_reduce()) {
                const auto& prod = productions.at(act.val);
                auto r = prod.rhs.size();
                if (prod.rhs.size() == 1 && prod.rhs[0].is_epsilon()) {
                    r = 0;
                }
                for (std::size_t i = 0; i < r; ++i) {
                    stack.pop();
                    stack.pop();
                }
                auto new_state = goto_table.at(stack.top().get_state()).at(prod.lhs);
                stack.push(prod.lhs);
                stack.push(new_state);
                output.emplace_back(prod);
                steps.add(prod, in.size() - pos);
#ifdef SHOW_DEBUG
                std::cout << prod << '\n';
#endif
            } else {
                auto errid = act.val;
                if (errid < error_handlers.size()) {
                    error_handlers[errid](stack, in, pos);
                } else {
                    throw exception::grammar_error("Unexpected token: " + cur_input.name + " at line " + std::to_string(in[pos].line) + ", column " + std::to_string(in[pos].column));
                }
            }
        }
    }

    void print_steps() const {
        steps.print();
    }

private:
    std::vector<items_t> items_set;
    std::vector<after_dot_t> after_dot_set;
    std::unordered_map<std::size_t, std::unordered_map<production::symbol, action>> action_table;
    std::unordered_map<std::size_t, std::unordered_map<production::symbol, std::size_t>> goto_table;
    rightmost_step steps;

    using error_handle_fn = std::function<void(std::stack<LR_stack_t>&, std::vector<lexer::token>&, std::size_t&)>;
    std::vector<error_handle_fn> error_handlers;

    void build() {
        calc_first();
        calc_follow();
        build_items_set();
#ifdef SHOW_DEBUG
        print_items_set();
        print_tables();
#endif
    }

    void build_items_set() {
        items_set.reserve(productions.size() * 2);
        after_dot_set.reserve(productions.size() * 2);

        std::unordered_set<production::symbol> after_dot;
        after_dot.insert(productions[0].rhs[0]);
        after_dot_set.emplace_back(std::move(after_dot));

        items_t initial_items{production::LR_production(productions[0])};
        add_closure(initial_items, 0);

        std::size_t init = 0;
        std::size_t end = items_set.size();
        while (init != end) {
            for (std::size_t i = init; i < end; i++) {
                after_dot = after_dot_set[i];
                for (const auto& sym : after_dot) {
                    move_dot(i, sym);
                }
            }
            init = end;
            end = items_set.size();
        }

        for (auto& action_table_it : action_table) {
            auto& col = action_table_it.second;
            if (!col.count(production::symbol{"}"})) {
                col[production::symbol{"}"}] = action::error(0);
            }
        }

        error_handlers.emplace_back([&](std::stack<LR_stack_t>& stack, std::vector<lexer::token>& in, std::size_t& pos) {
            std::cout << "语法错误，第" << in[pos].line - 1 << "行，缺少\";\"" << std::endl;
            in.insert(in.begin() + pos, lexer::token{";"});
            steps.insert_symbol(in.size() - pos - 1, production::symbol{";"});
        });
    }

    std::pair<bool, std::size_t> add_closure(items_t& current_items, std::size_t idx) {
        auto& after_dot = after_dot_set[idx];

        auto add = [&](const std::unordered_set<production::symbol>& symbols) {
            std::unordered_set<production::symbol> to_add;
            for (const auto& sym : symbols) {
                if (!sym.is_non_terminal()) {
                    continue;
                }
                for (auto id : symbol_map[sym]) {
                    auto lr_prod = production::LR_production(productions[id]);
                    current_items.insert(lr_prod);
                    if (lr_prod.is_end()) {
                        continue;
                    }
                    const auto& after_dot_sym = lr_prod.symbol_after_dot();
                    if (!after_dot.count(after_dot_sym)) {
                        to_add.insert(after_dot_sym);
                    }
                }
            }
            return to_add;
        };

        auto symbols = after_dot;
        bool is_empty;
        do {
            auto to_add = add(symbols);
            after_dot.insert(to_add.begin(), to_add.end());
            is_empty = to_add.empty();
            symbols = std::move(to_add);
        } while (!is_empty);

        if (current_items.empty()) {
            return {false, -1};
        }

        const auto res = std::find(items_set.begin(), items_set.end(), current_items);
        if (res != items_set.end()) {
            using diff_t = decltype(after_dot_set)::difference_type;
            assert(idx < after_dot_set.size() && idx < static_cast<std::size_t>(std::numeric_limits<diff_t>::max()));
            after_dot_set.erase(std::next(after_dot_set.begin(), static_cast<diff_t>(idx)));
            return {true, res - items_set.begin()};
        }

        for (const auto& item : current_items) {
            if (item.is_end()) {
                if (item.lhs == productions[0].lhs) {
                    assert(!action_table[idx].count(production::symbol::end_mark));
                    action_table[idx][production::symbol::end_mark] = action::accept();
                } else {
                    std::size_t pid = -1;
                    for (const auto id : symbol_map[item.lhs]) {
                        if (item == productions[id]) {
                            pid = id;
                            break;
                        }
                    }
                    assert(pid != -1);
                    for (const auto& s : follow[item.lhs]) {
                        assert(!action_table[idx].count(s));
                        action_table[idx][s] = action::reduce(pid);
                    }
                }
            }
        }

        items_set.emplace_back(std::move(current_items));
        return {true, items_set.size() - 1};
    }

    void move_dot(std::size_t idx, const production::symbol& sym) {
        const auto& items = items_set[idx];
        items_t new_items;
        after_dot_t after_dot;
        for (const auto& item : items) {
            if (!item.is_end() && item.symbol_after_dot() == sym) {
                auto next_item = item.next();
                new_items.insert(next_item);
                if (!next_item.is_end()) {
                    after_dot.insert(next_item.symbol_after_dot());
                }
            }
        }
        if (!new_items.empty()) {
            after_dot_set.emplace_back(std::move(after_dot));
            auto res = add_closure(new_items, after_dot_set.size() - 1);
            auto success = res.first;
            auto to = res.second;
            if (success) {
                if (sym.is_non_terminal()) {
                    goto_table[idx][sym] = to;
                } else if (sym.is_terminal() || sym.is_end_mark()) {
                    action_table[idx][sym] = action::shift(to);
                }
            }
        }
    }

    void print_items_set() const {
        int i = 0;
        for (const auto& items : items_set) {
            std::cout << "------------------------" << std::endl;
            std::cout << "I" << i++ << ":" << std::endl;
            for (const auto& item : items) {
                std::cout << item << std::endl;
            }
            std::cout << "After dot: ";
            for (const auto& sym : after_dot_set[i - 1]) {
                std::cout << sym << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "------------------------" << std::endl;
    }

    void print_tables() const {
        std::cout << "Parsing Table:\n";
        std::set<production::symbol> terminals;
        for (const auto& action_table_it : action_table) {
            auto row = action_table_it.second;
            for (const auto& row_it : row) {
                auto sym = row_it.first;
                terminals.insert(sym);
            }
        }

        std::set<production::symbol> non_terminals;
        for (const auto& goto_table_it : goto_table) {
            auto row = goto_table_it.second;
            for (const auto& row_it : row) {
                auto sym = row_it.first;
                non_terminals.insert(sym);
            }
        }

        std::cout << std::left << std::setw(8) << "state";
        for (const auto& sym : terminals) {
            std::cout << std::setw(8) << sym;
        }
        for (const auto& sym : non_terminals) {
            if (sym.name.size() > 5) {
                std::cout << std::setw(8) << sym.name.substr(0, 5) + "..";
            } else {
                std::cout << std::setw(8) << sym;
            }
        }
        std::cout << "\n";

        std::set<std::size_t> all_states;
        for (const auto& action_table_it : action_table) {
            auto state = action_table_it.first;
            auto _ = action_table_it.second;
            all_states.insert(state);
        }
        for (const auto& goto_table_it : goto_table) {
            auto state = goto_table_it.first;
            auto _ = goto_table_it.second;
            all_states.insert(state);
        }

        for (std::size_t state : all_states) {
            std::cout << std::left << std::setw(8) << state;

            auto action_row_it = action_table.find(state);
            for (const auto& sym : terminals) {
                if (action_row_it != action_table.end()) {
                    const auto& row = action_row_it->second;
                    auto it = row.find(sym);
                    if (it != row.end()) {
                        std::cout << std::setw(8) << it->second;
                    } else {
                        std::cout << std::setw(8) << "";
                    }
                } else {
                    std::cout << std::setw(8) << "";
                }
            }

            auto goto_row_it = goto_table.find(state);
            for (const auto& sym : non_terminals) {
                if (goto_row_it != goto_table.end()) {
                    const auto& row = goto_row_it->second;
                    auto it = row.find(sym);
                    if (it != row.end()) {
                        std::cout << std::setw(8) << it->second;
                    } else {
                        std::cout << std::setw(8) << "";
                    }
                } else {
                    std::cout << std::setw(8) << "";
                }
            }

            std::cout << "\n";
        }
    }
};
} // namespace grammar

#endif

// #define SHOW_DEBUG

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

const std::string epsilon_str = "E";

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

void Analysis() {
    string prog;
    read_prog(prog);
    /* 骚年们 请开始你们的表演 */
    /********* Begin *********/
    grammar::production::symbol::set_epsilon_str(epsilon_str);
    grammar::production::symbol::set_terminal_rule([&](const std::string& str) {
        return terminals.count(str);
    });

    grammar::SLR parser(gram);
    parser.parse(simple_lexer(prog));
    parser.print_steps();

    /********* End *********/
}

int main() {
    Analysis();
}
