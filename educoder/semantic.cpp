// C语言词法分析器
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
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
#define USE_STD_REGEX
#define SEMA_PROD_USE_INITIALIZER_LIST

// Begin include "grammar/production.hpp"
#ifndef GRAMMAR_PRODUCTION_HPP
#define GRAMMAR_PRODUCTION_HPP

// Begin include "../lexer/lexer.hpp"
#ifndef LEXER_LEXER_HPP
#define LEXER_LEXER_HPP

#ifdef USE_STD_REGEX
#include <regex>
#else
// Begin include "../regex/regex.hpp"
#ifndef REGEX_REGEX_HPP
#define REGEX_REGEX_HPP

// Begin include "dfa.hpp"

#ifndef REGEX_DFA_HPP
#define REGEX_DFA_HPP

// Begin include "token.hpp"

#ifndef REGEX_TOKEN_HPP
#define REGEX_TOKEN_HPP

// Begin include "exception.hpp"
#ifndef REGEX_EXCEPTION_HPP
#define REGEX_EXCEPTION_HPP

#include <exception>
#include <string>

namespace regex {
class unknown_character_exception final : public std::exception {
public:
    explicit unknown_character_exception(const std::string& ch)
        : character(ch), message("Unknown character: ") {
        message += character;
    }

    const char* what() const noexcept override {
        return message.c_str();
    }

private:
    std::string character;
    std::string message;
};

class invalid_regex_exception final : public std::exception {
public:
    explicit invalid_regex_exception(const std::string& mes)
        : message("Invalid regex: ") {
        message += mes;
    }

    const char* what() const noexcept override {
        return message.c_str();
    }

private:
    std::string message;
};
}; // namespace regex

#endif // REGEX_EXCEPTION_HPP// End include "exception.hpp"
// Begin include "token_type.hpp"

#include <initializer_list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace regex::token {
enum class symbol {
    end_mark
};

enum class op {
    concat,
    alt,
    star,
    plus,
    left_par,
    right_par,
    backslash
};

const std::unordered_map<symbol, std::string> symbol_map = {
    {symbol::end_mark, "#"}};

const std::unordered_map<op, std::string> op_map = {
    {op::concat, "·"},
    {op::alt, "|"},
    {op::star, "*"},
    {op::plus, "+"},
    {op::left_par, "("},
    {op::right_par, ")"},
    {op::backslash, "\\"}};

enum class nothing {};

struct char_set {
    std::unordered_set<char> chars;
    bool is_negative = false;

    char_set() = default;

    explicit char_set(const char from, const char to, const bool is_negative = false) : is_negative(is_negative) {
        for (char ch = from; ch <= to; ++ch) {
            chars.insert(ch);
        }
    }

    explicit char_set(const std::initializer_list<char> init, bool is_negative = false) : is_negative(is_negative) {
        for (const auto& ch : init) {
            chars.insert(ch);
        }
    }

    explicit char_set(std::initializer_list<std::pair<char, char>> init, const bool is_negative = false) : is_negative(is_negative) {
        for (const auto& _it_from : init) {
            auto& from = std::get<0>(_it_from);
            auto& to = std::get<1>(_it_from);
            add(from, to);
        }
    }

    void add(const char ch) {
        chars.insert(ch);
    }

    void add(const char_set& other) {
        chars.insert(other.chars.begin(), other.chars.end());
    }

    void add(const char from, const char to) {
        for (char ch = from; ch <= to; ++ch) {
            chars.insert(ch);
        }
    }

    bool operator==(const char_set& other) const {
        return chars == other.chars && is_negative == other.is_negative;
    }
};

static const char_set words{{'a', 'z'}, {'A', 'Z'}, {'0', '9'}, {'_', '_'}};
static const char_set digits('0', '9');
static const char_set whitespaces{' ', '\t', '\n', '\r', '\f', '\v'};

} // namespace regex::token

#if __cplusplus >= 201703L
#include <variant>
namespace regex::token {
using token_type = std::variant<nothing, symbol, op, char_set, char>;
}
#else
namespace regex::token {
struct token_type {
    enum class kind {
        nothing,
        symbol,
        op,
        char_set,
        character
    };

    kind type;

    union storage {
        nothing _nothing;
        symbol sym;
        op operation;
        char_set set;
        char character;

        storage() {}
        ~storage() {}
    } value;

    token_type() : type(kind::nothing) {
        new (&value._nothing) nothing{};
    }

    token_type(nothing) : type(kind::nothing) {
        new (&value._nothing) nothing{};
    }

    token_type(symbol s) : type(kind::symbol) {
        new (&value.sym) symbol(s);
    }

    token_type(op o) : type(kind::op) {
        new (&value.operation) op(o);
    }

    token_type(const char_set& cs) : type(kind::char_set) {
        new (&value.set) char_set(cs);
    }

    token_type(char c) : type(kind::character) {
        new (&value.character) char(c);
    }

    token_type(const token_type& other) : type(other.type) {
        switch (type) {
        case kind::nothing:
            new (&value._nothing) nothing{};
            break;
        case kind::symbol:
            new (&value.sym) symbol(other.value.sym);
            break;
        case kind::op:
            new (&value.operation) op(other.value.operation);
            break;
        case kind::char_set:
            new (&value.set) char_set(other.value.set);
            break;
        case kind::character:
            new (&value.character) char(other.value.character);
            break;
        }
    }

    token_type(token_type&& other) noexcept : type(other.type) {
        switch (type) {
        case kind::nothing:
            new (&value._nothing) nothing{};
            break;
        case kind::symbol:
            new (&value.sym) symbol(other.value.sym);
            break;
        case kind::op:
            new (&value.operation) op(other.value.operation);
            break;
        case kind::char_set:
            new (&value.set) char_set(std::move(other.value.set));
            break;
        case kind::character:
            new (&value.character) char(other.value.character);
            break;
        }
    }

    ~token_type() {
        destroy();
    }

    token_type& operator=(const token_type& other) {
        if (this != &other) {
            destroy();
            new (this) token_type(other);
        }
        return *this;
    }

    token_type& operator=(token_type&& other) noexcept {
        if (this != &other) {
            destroy();
            new (this) token_type(std::move(other));
        }
        return *this;
    }

    bool operator==(const token_type& other) const {
        if (type != other.type) return false;
        switch (type) {
        case kind::nothing:
            return true;
        case kind::symbol:
            return value.sym == other.value.sym;
        case kind::op:
            return value.operation == other.value.operation;
        case kind::char_set:
            return value.set == other.value.set;
        case kind::character:
            return value.character == other.value.character;
        default:
            return false;
        }
    }

private:
    void destroy() {
        switch (type) {
        case kind::char_set:
            value.set.~char_set();
            break;
        default:
            break; // trivial types don't need destruction
        }
    }
};
} // namespace regex::token

#include <stdexcept>
namespace std {
template <typename T>
struct token_type_traits;

template <>
struct token_type_traits<regex::token::nothing> {
    static constexpr regex::token::token_type::kind value = regex::token::token_type::kind::nothing;
};

template <>
struct token_type_traits<regex::token::symbol> {
    static constexpr regex::token::token_type::kind value = regex::token::token_type::kind::symbol;
};

template <>
struct token_type_traits<regex::token::op> {
    static constexpr regex::token::token_type::kind value = regex::token::token_type::kind::op;
};

template <>
struct token_type_traits<regex::token::char_set> {
    static constexpr regex::token::token_type::kind value = regex::token::token_type::kind::char_set;
};

template <>
struct token_type_traits<char> {
    static constexpr regex::token::token_type::kind value = regex::token::token_type::kind::character;
};

template <typename T>
bool holds_alternative(const regex::token::token_type& t) {
    return t.type == token_type_traits<T>::value;
}

template <typename T>
T& get(regex::token::token_type& t) {
    if (!holds_alternative<T>(t)) {
        throw std::bad_cast();
    }

    if (std::is_same<T, regex::token::nothing>::value) {
        return (T&)(t.value._nothing);
    }
    if (std::is_same<T, regex::token::symbol>::value) {
        return (T&)(t.value.sym);
    }
    if (std::is_same<T, regex::token::op>::value) {
        return (T&)(t.value.operation);
    }
    if (std::is_same<T, regex::token::char_set>::value) {
        return (T&)(t.value.set);
    }
    if (std::is_same<T, char>::value) {
        return (T&)(t.value.character);
    }

    throw std::bad_cast();
}

template <typename T>
const T& get(const regex::token::token_type& t) {
    return get<T>(const_cast<regex::token::token_type&>(t));
}

template <typename Visitor>
auto visit(Visitor&& vis, regex::token::token_type& t) -> decltype(auto) {
    using namespace regex::token;
    switch (t.type) {
    case token_type::kind::nothing:
        return vis(t.value._nothing);
    case token_type::kind::symbol:
        return vis(t.value.sym);
    case token_type::kind::op:
        return vis(t.value.operation);
    case token_type::kind::char_set:
        return vis(t.value.set);
    case token_type::kind::character:
        return vis(t.value.character);
    default:
        throw std::logic_error("Unknown token_type");
    }
}

template <typename Visitor>
auto visit(Visitor&& vis, const regex::token::token_type& t) -> decltype(auto) {
    return visit(std::forward<Visitor>(vis), const_cast<regex::token::token_type&>(t));
}

template <typename T>
T* get_if(regex::token::token_type* t) {
    using namespace regex::token;

    if (!t || !holds_alternative<T>(*t)) return nullptr;

    if (std::is_same<T, nothing>::value) {
        return (T*)(&t->value._nothing);
    }
    if (std::is_same<T, symbol>::value) {
        return (T*)(&t->value.sym);
    }
    if (std::is_same<T, op>::value) {
        return (T*)(&t->value.operation);
    }
    if (std::is_same<T, char_set>::value) {
        return (T*)(&t->value.set);
    }
    if (std::is_same<T, char>::value) {
        return (T*)(&t->value.character);
    }

    return nullptr;
}

template <typename T>
const T* get_if(const regex::token::token_type* t) {
    return get_if<T>(const_cast<regex::token::token_type*>(t));
}

} // namespace std
#endif// End include "token_type.hpp"

#include <initializer_list>
#include <iostream>
#include <ostream>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace regex::token {
struct token_type_hash {
    std::size_t operator()(const token_type& t) const {
        if (std::holds_alternative<char>(t)) {
            return std::hash<char>{}(std::get<char>(t));
        } else if (std::holds_alternative<symbol>(t)) {
            return std::hash<int>{}(static_cast<int>(std::get<symbol>(t)));
        } else if (std::holds_alternative<op>(t)) {
            return std::hash<int>{}(static_cast<int>(std::get<op>(t)));
        } else if (std::holds_alternative<char_set>(t)) {
            std::size_t hash = 0;
            const auto& set = std::get<char_set>(t);
            for (const auto& ch : set.chars) {
                hash ^= std::hash<char>{}(ch);
            }
            return hash;
        }
        return 0;
    }
};

int get_precedence(op opr) {
    switch (opr) {
    case op::star: return 3;
    case op::plus: return 3;
    case op::concat: return 2;
    case op::alt: return 1;
    case op::left_par: return 0;
    case op::right_par: return 0;
    case op::backslash: return -1;
    }
    return -1;
}

int get_precedence(const token_type& ch) {
    op opr;
    if (std::holds_alternative<op>(ch)) {
        opr = std::get<op>(ch);
    } else {
        return -1;
    }

    return get_precedence(opr);
}

bool is_char(const token_type& ch) {
    return std::holds_alternative<char>(ch);
}

bool is_symbol(const token_type& ch) {
    return std::holds_alternative<symbol>(ch);
}

bool is_op(const token_type& ch) {
    return std::holds_alternative<op>(ch);
}

bool is_char_set(const token_type& ch) {
    return std::holds_alternative<char_set>(ch);
}

template <typename T>
bool is(token_type ch, T other) {
    if (auto* p = std::get_if<T>(&ch)) {
        return *p == other;
    }
    return false;
}

bool match(char c, const token_type& ch) {
    if (is_char(ch)) {
        return std::get<char>(ch) == c;
    }
    if (is_char_set(ch)) {
        const auto& set = std::get<char_set>(ch);
        if (set.is_negative) {
            return set.chars.find(c) == set.chars.end();
        } else {
            return set.chars.find(c) != set.chars.end();
        }
    }
    return false;
}

bool is_nonop(const char ch) {
    return ch != '\\' && ch != '|' && ch != '*' && ch != '(' && ch != ')' && ch != '+' && ch != '[' && ch != ']';
}

bool is_nonop(const token_type& ch) {
    return is_char(ch) || is_symbol(ch) || is_char_set(ch);
}

std::vector<token_type> split(const std::string& s) {
    std::vector<token_type> result{op::left_par};
    token_type last = nothing{};
    bool in_char_set = false;
    bool in_range = false;
    char last_char_in_set = '\0';
    char_set current_set;
    for (const char& ch : s) {
        // add concat operator
        if (is_char(last) || is_symbol(last) || is_char_set(last) || is(last, op::right_par) || is(last, op::star) || is(last, op::plus)) {
            if (!in_char_set && (is_nonop(ch) || ch == '(' || ch == '\\' || ch == '[')) {
                result.push_back(op::concat);
            }
        }

        if (in_char_set) {
            if (ch == ']') {
                in_char_set = false;
                last = current_set;
                result.push_back(current_set);
                current_set = char_set{};
            } else if (ch == '-') {
                in_range = true;
            } else if (in_range) {
                in_range = false;
                current_set.add(last_char_in_set, ch);
            } else if (ch == '^') {
                current_set.is_negative = true;
            } else {
                current_set.add(ch);
            }
            if (!in_range) {
                last_char_in_set = ch;
            }
            continue;
        }

        if (is(last, op::backslash)) {
            if (!is_nonop(ch)) {
                last = ch;
            } else if (ch == 'w') {
                last = words;
            } else if (ch == 'd') {
                last = digits;
            } else if (ch == 's') {
                last = whitespaces;
            } else if (ch == '0') {
                last = '\0';
            } else if (ch == 'a') {
                last = '\a';
            } else if (ch == 'v') {
                last = '\v';
            } else if (ch == 'n') {
                last = '\n';
            } else if (ch == 't') {
                last = '\t';
            } else if (ch == 'r') {
                last = '\r';
            } else if (ch == 'f') {
                last = '\f';
            } else {
                throw regex::unknown_character_exception(std::string{'\\', ch});
            }
        } else {
            if (is_nonop(ch)) {
                last = ch;
            } else if (ch == '\\') {
                last = op::backslash;
                continue;
            } else if (ch == '|') {
                last = op::alt;
            } else if (ch == '*') {
                last = op::star;
            } else if (ch == '+') {
                last = op::plus;
            } else if (ch == '(') {
                last = op::left_par;
            } else if (ch == ')') {
                last = op::right_par;
            } else if (ch == '[') {
                in_char_set = true;
                continue;
            } else {
                throw regex::unknown_character_exception(std::string{ch});
            }
        }
        result.push_back(last);
    }

    if (in_char_set) {
        throw regex::invalid_regex_exception("Unmatched '[' in regex");
    }
    if (in_range) {
        throw regex::invalid_regex_exception("Unmatched '-' in regex");
    }

    if (!result.empty()) {
        result.push_back(op::right_par);
        result.push_back(op::concat);
        result.push_back(symbol::end_mark);
    }

    return result;
}

std::vector<token_type> to_postfix(const std::vector<token_type>& v) {
    std::vector<token_type> res;
    std::stack<token_type> ops;

    for (const auto& ch : v) {
        if (is_char(ch) || is_symbol(ch) || is_char_set(ch)) {
            res.push_back(ch);
        } else if (is(ch, op::left_par)) {
            ops.push(ch);
        } else if (is(ch, op::right_par)) {
            while (!ops.empty() && !is(ops.top(), op::left_par)) {
                res.push_back(ops.top());
                ops.pop();
            }
            ops.pop();
        } else if (is(ch, op::alt)) {
            while (!ops.empty() && get_precedence(ops.top()) >= get_precedence(op::alt)) {
                res.push_back(ops.top());
                ops.pop();
            }
            ops.push(ch);
        } else if (is(ch, op::concat)) {
            while (!ops.empty() && get_precedence(ops.top()) >= get_precedence(op::concat)) {
                res.push_back(ops.top());
                ops.pop();
            }
            ops.push(ch);
        } else if (is(ch, op::star) || is(ch, op::plus)) {
            ops.push(ch);
        } else {
            throw regex::unknown_character_exception(std::string{std::get<char>(ch)});
        }
    }

    while (!ops.empty()) {
        res.push_back(ops.top());
        ops.pop();
    }

    return res;
}

static const std::unordered_map<char, std::string> escape_map = {
    {'\0', "\\0"},
    {'\a', "\\a"},
    {'\v', "\\v"},
    {'\n', "\\n"},
    {'\t', "\\t"},
    {'\r', "\\r"},
    {'\f', "\\f"}};

std::ostream& operator<<(std::ostream& os, const token_type& ch) {
    if (is_char(ch)) {
        char c = std::get<char>(ch);
        if (escape_map.find(c) != escape_map.end()) {
            os << escape_map.at(c);
        } else if (std::isprint(c)) {
            os << c;
        } else {
            os << '\\' << static_cast<int>(c);
        }
    } else if (is_op(ch)) {
        os << op_map.at(std::get<op>(ch));
    } else if (is_symbol(ch)) {
        os << symbol_map.at(std::get<symbol>(ch));
    } else if (is_char_set(ch)) {
        const auto& set = std::get<char_set>(ch);
        std::set<char> new_chars;
        bool is_negative = set.is_negative;

        if (set.chars.size() > 128 / 2) {
            is_negative = !is_negative;
            for (int i = 0; i < 128; ++i) {
                char c = static_cast<char>(i);
                if (set.chars.find(c) == set.chars.end()) {
                    new_chars.insert(c);
                }
            }
        } else {
            new_chars.insert(set.chars.begin(), set.chars.end());
        }

        if (is_negative) {
            os << "[^";
        } else {
            os << '[';
        }

        if (new_chars.empty()) {
            os << ']';
            return os;
        }

        // for (auto it = new_chars.begin(); it != new_chars.end(); ++it) {
        //     os << *it;
        // }

        auto it = new_chars.begin();
        char start = *it;
        char prev = *it;

        ++it;
        for (; it != new_chars.end(); ++it) {
            if (*it == prev + 1) {
                prev = *it;
            } else {
                if (start == prev) {
                    os << start;
                } else if (prev == start + 1) {
                    os << start << prev;
                } else {
                    os << start << '-' << prev;
                }
                start = prev = *it;
            }
        }

        if (start == prev) {
            os << start;
        } else if (prev == start + 1) {
            os << start << prev;
        } else {
            os << start << '-' << prev;
        }

        os << ']';
    } else {
        throw std::invalid_argument("Invalid token type for output stream");
    }
    return os;
}

void print(const std::vector<token_type>& v) {
    for (const auto& ch : v) {
        std::cout << ch << ' ';
    }
    std::cout << std::endl;
}

char_set to_char_set(const token_type& token) {
    if (std::holds_alternative<char>(token)) {
        return char_set{std::get<char>(token)};
    } else if (std::holds_alternative<char_set>(token)) {
        return std::get<char_set>(token);
    } else {
        throw std::invalid_argument("Invalid token type for conversion to char_set");
    }
}
} // namespace regex::token

#endif // REGEX_TOKEN_HPP// End include "token.hpp"
// Begin include "tree.hpp"

#ifndef REGEX_TREE_HPP
#define REGEX_TREE_HPP

// Begin include "exception.hpp"
// End include "exception.hpp"
// Begin include "token.hpp"
// End include "token.hpp"

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace regex::tree {

class regex_node {
public:
    enum class type {
        ch,
        concat,
        alt,
        star,
        plus,
    };

    using node_ptr_t = std::shared_ptr<regex_node>;

    type type;

    bool nullable;
    std::unordered_set<std::size_t> firstpos;
    std::unordered_set<std::size_t> lastpos;

    virtual ~regex_node() = default;

    template <class T>
    T& as() {
        return static_cast<T&>(*this);
    }
};

class char_node final : public regex_node {
public:
    token::token_type value;
    std::size_t number;

    explicit char_node(const token::token_type& ch, const std::size_t number) : value(ch), number(number) {
        type = type::ch;
        nullable = false;
        firstpos.insert(number);
        lastpos.insert(number);
    }
};

class concat_node final : public regex_node {
public:
    node_ptr_t left, right;
    explicit concat_node(const node_ptr_t& left, const node_ptr_t& right) : left(left), right(right) {
        type = type::concat;
        nullable = left->nullable && right->nullable;
        firstpos = left->firstpos;
        if (left->nullable) {
            firstpos.insert(right->firstpos.begin(), right->firstpos.end());
        }
        lastpos = right->lastpos;
        if (right->nullable) {
            lastpos.insert(left->lastpos.begin(), left->lastpos.end());
        }
    }
};

class star_node final : public regex_node {
public:
    node_ptr_t child;
    explicit star_node(const node_ptr_t& child) : child(child) {
        type = type::star;
        nullable = true;
        firstpos = child->firstpos;
        lastpos = child->lastpos;
    }
};

class plus_node final : public regex_node {
public:
    node_ptr_t child;
    explicit plus_node(const node_ptr_t& child) : child(child) {
        type = type::plus;
        nullable = child->nullable;
        firstpos = child->firstpos;
        lastpos = child->lastpos;
    }
};

class alt_node final : public regex_node {
public:
    node_ptr_t left, right;
    explicit alt_node(const node_ptr_t& left, const node_ptr_t& right) : left(left), right(right) {
        type = type::alt;
        nullable = left->nullable || right->nullable;
        firstpos = left->firstpos;
        firstpos.insert(right->firstpos.begin(), right->firstpos.end());
        lastpos = left->lastpos;
        lastpos.insert(right->lastpos.begin(), right->lastpos.end());
    }
};

class regex_tree {
public:
    regex_node::node_ptr_t root;

    using token_map_t = std::unordered_map<token::token_type, std::unordered_set<std::size_t>, token::token_type_hash>;

    token_map_t token_map;

    std::unordered_map<std::size_t, std::unordered_set<std::size_t>> followpos;

    explicit regex_tree(regex_node& root) : root(&root) {}

    explicit regex_tree(const std::string& s) {
        auto ss = token::split(s);
#ifdef SHOW_DEBUG
        ::regex::token::print(ss);
#endif
        auto postfix = to_postfix(ss);
#ifdef SHOW_DEBUG
        ::regex::token::print(postfix);
#endif

        std::stack<regex_node::node_ptr_t> st;

        size_t i = 1;

        using token::op;

        for (const auto& ch : postfix) {
            if (is(ch, op::star)) {
                if (st.empty()) {
                    throw regex::invalid_regex_exception("'*' operator with empty stack");
                }
                auto operand = st.top();
                st.pop();
                st.push(std::make_shared<star_node>(star_node{operand}));
            } else if (is(ch, op::plus)) {
                if (st.empty()) {
                    throw regex::invalid_regex_exception("'+' operator with empty stack");
                }
                auto operand = st.top();
                st.pop();
                st.push(std::make_shared<plus_node>(plus_node{operand}));
            } else if (is(ch, op::concat)) {
                if (st.size() < 2) {
                    throw regex::invalid_regex_exception("'·' operator with fewer than 2 operands");
                }
                auto right = st.top();
                st.pop();
                auto left = st.top();
                st.pop();
                st.push(std::make_shared<concat_node>(concat_node{left, right}));
            } else if (is(ch, op::alt)) {
                if (st.size() < 2) {
                    throw regex::invalid_regex_exception("'|' operator with fewer than 2 operands");
                }
                auto right = st.top();
                st.pop();
                auto left = st.top();
                st.pop();
                st.push(std::make_shared<alt_node>(alt_node{left, right}));
            } else if (is_nonop(ch)) {
                token_map[ch].insert(i);
                st.push(std::make_shared<char_node>(char_node{ch, i++}));
            } else {
                throw regex::invalid_regex_exception(s);
            }
        }

        if (st.size() != 1) {
            throw regex::invalid_regex_exception("leftover operands after parsing");
        }

        root = st.top();
        st.pop();

        visit([&](regex_node& node) {
            if (node.type == regex_node::type::concat) {
                auto& concat_node = node.as<tree::concat_node>();
                for (auto idx : concat_node.left->lastpos) {
                    followpos[idx].insert(concat_node.right->firstpos.begin(), concat_node.right->firstpos.end());
                }
            } else if (node.type == regex_node::type::star) {
                auto& star_node = node.as<tree::star_node>();
                for (auto idx : star_node.lastpos) {
                    followpos[idx].insert(star_node.firstpos.begin(), star_node.firstpos.end());
                }
            } else if (node.type == regex_node::type::plus) {
                auto& plus_node = node.as<tree::plus_node>();
                for (auto idx : plus_node.lastpos) {
                    followpos[idx].insert(plus_node.firstpos.begin(), plus_node.firstpos.end());
                }
            }
        });

        token_map = disjoint_token_sets(token_map);
    }

    void visit(const std::function<void(regex_node&)>& func) const {
        visit(root, func);
    }

    void print() const {
        std::cout << "Regex Tree" << std::endl;
        print(root);
    }

private:
    static void visit(const regex_node::node_ptr_t& node, const std::function<void(regex_node&)>& func) {
        if (node == nullptr) {
            return;
        }

        func(*node.get());

        if (node->type == regex_node::type::concat) {
            auto& concat_node = node->as<tree::concat_node>();
            visit(concat_node.left, func);
            visit(concat_node.right, func);
        } else if (node->type == regex_node::type::alt) {
            auto& alt_node = node->as<tree::alt_node>();
            visit(alt_node.left, func);
            visit(alt_node.right, func);
        } else if (node->type == regex_node::type::star) {
            auto& star_node = node->as<tree::star_node>();
            visit(star_node.child, func);
        } else if (node->type == regex_node::type::plus) {
            auto& plus_node = node->as<tree::plus_node>();
            visit(plus_node.child, func);
        }
    }

    static void print(const regex_node::node_ptr_t& node, const int indent = 0) {
        using token::op;

        if (node == nullptr) {
            return;
        }
        std::cout << std::string(indent, ' ');
        if (node->type == regex_node::type::ch) {
            const auto& char_node = node->as<tree::char_node>();
            std::cout << char_node.value << '(' << char_node.number << ')' << std::endl;
        } else if (node->type == regex_node::type::concat) {
            std::cout << token::op_map.at(op::concat) << std::endl;
            print(node->as<concat_node>().left, indent + 2);
            print(node->as<concat_node>().right, indent + 2);
        } else if (node->type == regex_node::type::alt) {
            std::cout << token::op_map.at(op::alt) << std::endl;
            print(node->as<alt_node>().left, indent + 2);
            print(node->as<alt_node>().right, indent + 2);
        } else if (node->type == regex_node::type::star) {
            std::cout << token::op_map.at(op::star) << std::endl;
            print(node->as<star_node>().child, indent + 2);
        } else if (node->type == regex_node::type::plus) {
            std::cout << token::op_map.at(op::plus) << std::endl;
            print(node->as<plus_node>().child, indent + 2);
        }
    }

    struct unordered_set_hash {
        std::size_t operator()(const std::unordered_set<std::size_t>& s) const {
            std::size_t h = 0;
            for (const auto v : s) {
                h ^= std::hash<std::size_t>{}(v);
            }
            return h;
        }
    };

    static token_map_t disjoint_token_sets(const token_map_t& original) {
        std::unordered_map<char, std::unordered_set<size_t>> char_to_positions;

        for (const auto& _it_token : original) {
            auto& token = std::get<0>(_it_token);
            auto& positions = std::get<1>(_it_token);
            if (is(token, token::symbol::end_mark)) {
                continue;
            }
            auto cs = to_char_set(token);
            if (!cs.is_negative) {
                for (char ch : cs.chars) {
                    char_to_positions[ch].insert(positions.begin(), positions.end());
                }
            } else {
                for (int c = -128; c < 128; ++c) {
                    if (!cs.chars.count(static_cast<char>(c))) {
                        char_to_positions[c].insert(positions.begin(), positions.end());
                    }
                }
            }
        }

        std::unordered_map<std::unordered_set<size_t>, std::unordered_set<char>, unordered_set_hash> grouped;
        for (const auto& _it_ch : char_to_positions) {
            auto& ch = std::get<0>(_it_ch);
            auto& pos = std::get<1>(_it_ch);
            grouped[pos].insert(ch);
        }

        token_map_t result;

        for (const auto& _it_positions : grouped) {
            auto& positions = std::get<0>(_it_positions);
            auto& chars = std::get<1>(_it_positions);
            if (chars.size() == 1) {
                result[token::token_type{*chars.begin()}] = positions;
                continue;
            }
            token::char_set cs;
            for (char ch : chars) {
                cs.add(ch);
            }
            result[token::token_type{cs}] = positions;
        }

        result[token::token_type{token::symbol::end_mark}] = original.at(token::symbol::end_mark);

        return result;
    }
};

} // namespace regex::tree

#endif // REGEX_TREE_HPP
// End include "tree.hpp"

#include <cstddef>
#include <unordered_set>

namespace regex::dfa {
class dfa {
public:
    using state_t = std::size_t;
    using token_t = token::token_type;
    using token_t_hash = token::token_type_hash;
    using transition_t = std::unordered_map<token_t, state_t, token_t_hash>;
    using dfa_state_t = std::unordered_map<state_t, transition_t>;

    dfa() = delete;

    explicit dfa(const tree::regex_tree& tree) {
        init(tree);
    }

    explicit dfa(const std::string& regex) {
        tree::regex_tree tree(regex);
#ifdef SHOW_DEBUG
        tree.print();
#endif
        init(tree);
#ifdef SHOW_DEBUG
        this->print();
#endif
    }

    void add_transition(const state_t from, const token_t& token, const state_t to) {
        transitions[from][token] = to;
    }

    bool match(const std::string& str) const {
        state_t current_state = 1;
        for (const auto& ch : str) {
            bool find = false;
            if (!transitions.count(current_state)) {
                return false;
            }
            if (transitions.at(current_state).count(ch)) {
                current_state = transitions.at(current_state).at(ch);
                continue;
            }
            for (const auto& _it_token : transitions.at(current_state)) {
                auto& token = std::get<0>(_it_token);
                auto& to = std::get<1>(_it_token);
                if (token::match(ch, token)) {
                    current_state = to;
                    find = true;
                    break;
                }
            }
            if (!find) {
                return false;
            }
        }
        return accept_states.count(current_state) > 0;
    }

    std::size_t match_max(const std::string& str) const {
        state_t current_state = 1;
        std::size_t last_accept_pos = 0;

        for (size_t i = 0; i < str.size(); ++i) {
            char ch = str[i];
            bool find = false;

            if (!transitions.count(current_state)) {
                break;
            }

            if (transitions.at(current_state).count(ch)) {
                current_state = transitions.at(current_state).at(ch);
                find = true;
            } else {
                for (const auto& _it_token : transitions.at(current_state)) {
                    auto& token = std::get<0>(_it_token);
                    auto& to = std::get<1>(_it_token);
                    if (token::match(ch, token)) {
                        current_state = to;
                        find = true;
                        break;
                    }
                }
            }

            if (!find) {
                break;
            }

            if (accept_states.count(current_state)) {
                last_accept_pos = i + 1;
            }
        }

        return last_accept_pos;
    }

    const dfa_state_t& get_transitions() const {
        return transitions;
    }

    void print() const {
        std::cout << "DFA" << std::endl;
        for (const auto& _it_from : transitions) {
            auto& from = std::get<0>(_it_from);
            auto& trans = std::get<1>(_it_from);
            for (const auto& _it_token : trans) {
                auto& token = std::get<0>(_it_token);
                auto& to = std::get<1>(_it_token);
                std::cout << "  Transition: " << from << " -- " << token << " --> " << to << std::endl;
            }
        }
        std::cout << "  Accept states: ";
        for (const auto& state : accept_states) {
            std::cout << state << " ";
        }
        std::cout << std::endl;
    }

private:
    dfa_state_t transitions;
    std::unordered_set<state_t> accept_states;

    struct d_state_t {
        std::unordered_set<state_t> states;
        state_t id;

        bool operator==(const d_state_t& other) const {
            return states == other.states;
        }
    };

    struct d_state_t_hash {
        std::size_t operator()(const d_state_t& s) const {
            std::size_t h = 0;
            for (auto v : s.states) {
                h ^= std::hash<state_t>{}(v);
            }
            return h;
        }
    };

    void init(const tree::regex_tree& tree) {
        std::unordered_set<d_state_t, d_state_t_hash> d_states;
        size_t cur = 1;
        d_states.insert({std::unordered_set<state_t>(tree.root->firstpos.begin(), tree.root->firstpos.end()), cur++});

        std::unordered_set<d_state_t, d_state_t_hash> unmarked_d_states = d_states;

        std::unordered_set<token_t, token_t_hash> all_tokens;
        for (const auto& token : tree.token_map) {
            all_tokens.insert(token.first);
        }

        auto& token_map = tree.token_map;
        auto& followpos = tree.followpos;

        while (!unmarked_d_states.empty()) {
            auto d_state = *unmarked_d_states.begin();
            unmarked_d_states.erase(unmarked_d_states.begin());

            for (const auto& token : all_tokens) {
                if (token::is(token, token::symbol::end_mark)) {
                    continue;
                }

                d_state_t u;
                for (auto i : token_map.at(token)) {
                    if (d_state.states.count(i)) {
                        u.states.insert(followpos.at(i).begin(), followpos.at(i).end());
                    }
                }

                if (u.states.empty()) {
                    continue;
                }

                auto it = d_states.find(u);
                if (it == d_states.end()) {
                    u.id = cur++;
                    d_states.insert(u);
                    unmarked_d_states.insert(u);
                } else {
                    u.id = it->id;
                }

                if (u.states.count(*token_map.at(token::symbol::end_mark).begin())) {
                    accept_states.insert(u.id);
                }

                add_transition(d_state.id, token, u.id);
            }
        }
    }
};
} // namespace regex::dfa

#endif // REGEX_DFA_HPP// End include "dfa.hpp"

namespace regex {
class regex {
public:
    regex() = delete;
    explicit regex(const std::string& regex) : dfa_(regex) {}

    bool match(const std::string& str) const {
        return dfa_.match(str);
    }

    std::size_t match_max(const std::string& str) const {
        return dfa_.match_max(str);
    }

private:
    dfa::dfa dfa_;
};
}; // namespace regex

#endif // REGEX_REGEX_HPP// End include "../regex/regex.hpp"
#endif

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

namespace lexer {

#ifdef USE_STD_REGEX
class regex_wrapper {
public:
    explicit regex_wrapper(const std::string& pattern) : regex_(pattern) {}
    
    std::size_t match_max(const std::string& input) const {
        std::smatch match;
        if (std::regex_search(input, match, regex_, std::regex_constants::match_continuous)) {
            return match.length();
        }
        return 0;
    }

private:
    std::regex regex_;
};
#else
using regex_wrapper = regex::regex;
#endif

struct token {
    int type;
    std::string value;
    std::size_t line;
    std::size_t column;

    template <typename TokenType>
    token(const TokenType type, std::string value, const std::size_t line, const std::size_t column)
        : type(static_cast<int>(type)), value(std::move(value)), line(line), column(column) {
        static_assert(std::is_enum<TokenType>::value || std::is_convertible<TokenType, int>::value, "token_type must be an enum type");
    }

    explicit token(std::string value) : type(-1), value(std::move(value)), line(0), column(0) {}

    explicit operator std::string() const;
};

class lexer {
public:
    using tokens_t = std::vector<token>;
    using keyword_t = std::pair<regex_wrapper, int>;

    template <typename TokenType>
    struct input_keyword {
        std::string pattern_str;  // Store pattern as string for both regex types
        TokenType token;
        std::string name;
        
        // Constructor that accepts string pattern
        input_keyword(const std::string& pattern, TokenType tok, const std::string& n)
            : pattern_str(pattern), token(tok), name(n) {}
    };

    template <typename TokenType>
    using input_keywords_t = std::vector<input_keyword<TokenType>>;

    static int whitespace;
    static std::unordered_map<int, std::string> token_names;

    template <typename TokenType>
    lexer(const input_keywords_t<TokenType> key_words, TokenType whitespace_) {
        static_assert(std::is_enum<TokenType>::value, "token_type must be an enum type");
        whitespace = static_cast<int>(whitespace_);

        for (const auto& keyword : key_words) {
            const auto& pattern_str = keyword.pattern_str;
            const auto& token = keyword.token;
            const auto& name = keyword.name;
            token_names.insert({static_cast<int>(token), name});
            this->key_words.emplace_back(regex_wrapper(pattern_str), static_cast<int>(token));
        }
    }

    tokens_t parse(const std::string& input, bool skip_whitespace = true) const {
        std::size_t max_match = 0;
        std::string cur = input;
        int cur_token;

        std::size_t line = 0;
        std::size_t col = 0;

        tokens_t tokens;

        while (max_match < cur.size()) {
            for (auto& _it_pattern : key_words) {
                auto& pattern = std::get<0>(_it_pattern);
                auto& token = std::get<1>(_it_pattern);
                const auto match = pattern.match_max(cur);
                if (match > max_match) {
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

private:
    std::vector<keyword_t> key_words;
};

std::unordered_map<int, std::string> lexer::token_names{};

int lexer::whitespace;

token::operator std::string() const {
    if (type == -1) {
        return value;
    }
    return lexer::token_names[type];
}

std::ostream& operator<<(std::ostream& os, const token& t) {
    os << "Token(" << lexer::token_names.at(t.type) << ", \"" << t.value << "\", line: " << t.line << ", column: " << t.column << ")";
    return os;
}

} // namespace lexer

#endif // LEXER_LEXER_HPP// End include "../lexer/lexer.hpp"
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
    std::string lexval;

    std::size_t line = 0;
    std::size_t column = 0;

    static std::string epsilon_str;
    static std::string end_mark_str;

    static symbol epsilon;
    static symbol end_mark;

    static std::function<bool(const std::string&)> terminal_rule;

    symbol() {
        type = type::epsilon;
        name = std::string(epsilon_str);
        lexval = std::string(epsilon_str);
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
        lexval = trimed;
    }

    explicit symbol(const lexer::token& token) : symbol(std::string(token)) {
        update(token);
    }

    void update(const lexer::token& token) {
        this->lexval = token.value;
        this->line = token.line;
        this->column = token.column;
    }

    void update(const symbol& other) {
        this->lexval = other.lexval;
        this->line = other.line;
        this->column = other.column;
    }

    void update_pos(const symbol& other) {
        this->line = other.line;
        this->column = other.column;
    }

    [[nodiscard]] std::string to_string() const {
        return name;
    }

    [[nodiscard]] std::string get_lexval() const {
        return lexval;
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
} // namespace grammar::production

namespace std {
template <>
struct hash<grammar::production::symbol> {
    std::size_t operator()(const grammar::production::symbol& sym) const noexcept {
        return std::hash<std::string>()(sym.name);
    }
};
} // namespace std

namespace grammar::production {
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

std::ostream& operator<<(std::ostream& os, const production& prod) {
    os << prod.to_string();
    return os;
}

class LR_production : public production {
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

class LR1_production final : public LR_production {
public:
    symbol lookahead;

    LR1_production() = default;

    explicit LR1_production(const production& prod) {
        throw std::invalid_argument("LR1_production requires a lookahead symbol");
    }

    explicit LR1_production(const production& prod, symbol&& lookahead) : LR_production(prod), lookahead(std::move(lookahead)) {}

    explicit LR1_production(const production& prod, const symbol& lookahead) : LR_production(prod), lookahead(lookahead) {}

    void set_lookahead(const symbol& lookahead) {
        this->lookahead = lookahead;
    }

    [[nodiscard]] LR1_production next() const {
        LR1_production next = *this;
        if (dot_pos < rhs.size()) {
            ++next.dot_pos;
        }
        return next;
    }

    [[nodiscard]] std::string to_string() const override {
        return LR_production::to_string() + ", " + lookahead.name;
    }

    bool operator==(const LR1_production& other) const {
        return LR_production::operator==(other) && lookahead == other.lookahead;
    }

    bool operator==(const production& other) const override {
        return LR_production::operator==(other);
    }
};
} // namespace grammar::production

namespace std {
template <>
struct hash<grammar::production::production> {
    std::size_t operator()(const grammar::production::production& prod) const noexcept {
        std::size_t h = std::hash<grammar::production::symbol>()(prod.lhs);
        for (const auto& sym : prod.rhs) {
            h ^= std::hash<grammar::production::symbol>()(sym) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
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

template <>
struct hash<grammar::production::LR1_production> {
    std::size_t operator()(const grammar::production::LR1_production& lr1_prod) const noexcept {
        std::size_t h = std::hash<grammar::production::LR_production>()(lr1_prod);
        h ^= std::hash<grammar::production::symbol>()(lr1_prod.lookahead) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};
} // namespace std

namespace grammar {
void set_epsilon_str(const std::string& str) {
    using namespace grammar::production;
    symbol::epsilon_str = str;
    symbol::epsilon = symbol(symbol::epsilon_str);
}

void set_end_mark_str(const std::string& str) {
    using namespace grammar::production;
    symbol::end_mark_str = str;
    symbol::end_mark = symbol(symbol::end_mark_str);
}

void set_terminal_rule(const std::function<bool(const std::string&)>& rule) {
    using namespace grammar::production;
    symbol::terminal_rule = rule;
}

void set_terminal_rule(std::function<bool(const std::string&)>&& rule) {
    using namespace grammar::production;
    symbol::terminal_rule = std::move(rule);
}
} // namespace grammar

#endif // GRAMMAR_PRODUCTION_HPP// End include "grammar/production.hpp"
// Begin include "lexer/lexer.hpp"
// End include "lexer/lexer.hpp"
// Begin include "semantic/sema_grammar.hpp"
// Begin include "../grammar/grammar.hpp"
#ifndef GRAMMAR_GRAMMAR_HPP
#define GRAMMAR_GRAMMAR_HPP

// #include "LL1.hpp"          // IWYU pragma: export
// #include "LR1.hpp"          // IWYU pragma: export
// Begin include "SLR.hpp"
#ifndef GRAMMAR_SLR_HPP
#define GRAMMAR_SLR_HPP

// Begin include "exception.hpp"
#ifndef GRAMMAR_EXCEPTION_HPP
#define GRAMMAR_EXCEPTION_HPP

// Begin include "production.hpp"
// End include "production.hpp"
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

#endif // GRAMMAR_EXCEPTION_HPP// End include "exception.hpp"
// Begin include "grammar_base.hpp"
#include <memory>
#ifndef GRAMMAR_BASE_HPP
#define GRAMMAR_BASE_HPP

// Begin include "../lexer/lexer.hpp"
// End include "../lexer/lexer.hpp"
// Begin include "production.hpp"
// End include "production.hpp"
// Begin include "tree.hpp"

#ifndef GRAMMAR_TREE_HPP
#define GRAMMAR_TREE_HPP

// Begin include "production.hpp"
// End include "production.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

namespace grammar {
struct tree_node {
    std::shared_ptr<production::symbol> symbol;
    std::vector<std::shared_ptr<tree_node>> children;
    std::shared_ptr<tree_node> parent = nullptr;

    virtual ~tree_node() = default;

    explicit tree_node(const production::symbol& sym) : symbol(std::make_shared<production::symbol>(sym)) {}

    explicit tree_node(std::shared_ptr<production::symbol> sym) : symbol(sym) {}
};

class tree {
protected:
    std::shared_ptr<tree_node> root = nullptr;
    std::shared_ptr<tree_node> next = nullptr;
    std::shared_ptr<tree_node> next_r = nullptr;

    std::vector<std::shared_ptr<production::symbol>> to_replace;
    std::size_t replace_r_idx = 0;

public:
    tree() = default;

    virtual ~tree() = default;

    virtual void add(const production::production& prod) {
        if (!root) {
            root = std::make_shared<tree_node>(prod.lhs);
            std::vector<std::shared_ptr<production::symbol>> tmp;
            for (const auto& rhs : prod.rhs) {
                auto node = std::make_shared<tree_node>(rhs);
                if (rhs.is_terminal()) {
                    tmp.emplace_back(node->symbol);
                }
                node->parent = root;
                root->children.push_back(node);
                if (!next && rhs.is_non_terminal()) {
                    next = node;
                }
            }
            for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) {
                to_replace.push_back(*it);
            }
            for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
                if ((*it)->symbol->is_non_terminal()) {
                    next_r = *it;
                    break;
                }
            }
            return;
        }
        assert(next != nullptr);
        assert(next->children.empty());
        assert(*next->symbol == prod.lhs);
        bool found = false;
        std::shared_ptr<tree_node> new_next = nullptr;
        std::vector<std::shared_ptr<production::symbol>> tmp;
        for (const auto& rhs : prod.rhs) {
            auto node = std::make_shared<tree_node>(rhs);
            if (rhs.is_terminal()) {
                tmp.emplace_back(node->symbol);
            }
            node->parent = next;
            next->children.push_back(node);
            if (!found && rhs.is_non_terminal()) {
                new_next = node;
                found = true;
            }
        }
        for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) {
            to_replace.push_back(*it);
        }

        if (found) {
            next = new_next;
        } else {
            next = next->parent;
            while (next) {
                for (const auto& child : next->children) {
                    if (child->symbol->is_non_terminal() && child->children.empty()) {
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

    virtual void add_r(const production::production& prod) {
        if (!root) {
            root = std::make_shared<tree_node>(prod.lhs);
            for (const auto& rhs : prod.rhs) {
                auto node = std::make_shared<tree_node>(rhs);
                node->parent = root;
                root->children.push_back(node);
            }
            for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
                if ((*it)->symbol->is_non_terminal()) {
                    next_r = *it;
                    break;
                }
            }
            return;
        }

        assert(next_r != nullptr);
        assert(next_r->children.empty());
        assert(*next_r->symbol == prod.lhs);

        for (const auto& rhs : prod.rhs) {
            auto node = std::make_shared<tree_node>(rhs);
            node->parent = next_r;
            next_r->children.push_back(node);
        }

        bool found = false;

        for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
            if ((*it)->symbol->is_non_terminal()) {
                next_r = *it;
                found = true;
                break;
            }
        }

        if (!found) {
            next_r = next_r->parent;
            while (next_r) {
                for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
                    if ((*it)->symbol->is_non_terminal() && (*it)->children.empty()) {
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

    void update(const production::symbol& sym) {
        if (to_replace.empty()) {
            return;
        }
        auto back = to_replace.back();
        if (sym == *back) {
            back->update(sym);
            to_replace.pop_back();
        }
    }

    void update_r(const production::symbol& sym) {
        if (to_replace.empty()) {
            visit([&](std::shared_ptr<tree_node> node) {
                if (node->symbol && node->symbol->is_terminal() && !node->symbol->is_epsilon()) {
                    to_replace.emplace_back(node->symbol);
                }
            });
        }
        if (replace_r_idx >= to_replace.size()) {
            return;
        }
        auto ori = to_replace[replace_r_idx];
        if (sym == *ori) {
            ori->update(sym);
            replace_r_idx++;
        }
    }

    void print() const {
        if (root) {
            print_node(root, 0);
        }
    }

    virtual void print_node(const std::shared_ptr<tree_node>& node, const int depth) const {
        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }
        if (node->symbol) {
            std::cout << *node->symbol << "\n";
        }
        for (const auto& child : node->children) {
            print_node(child, depth + 1);
        }
    }

    void visit(const std::function<void(std::shared_ptr<tree_node>)>& func) {
        visit(root, func);
    }

private:
    void visit(std::shared_ptr<tree_node> node, const std::function<void(std::shared_ptr<tree_node>)>& func) {
        if (!node) {
            return;
        }
        func(node);
        if (node->children.empty()) {
            return;
        }
        for (auto child : node->children) {
            visit(child, func);
        }
    }
};
} // namespace grammar

#endif // GRAMMAR_TREE_HPP// End include "tree.hpp"
#include <stack>
#include <unordered_set>

namespace grammar {
class grammar_base {
public:
    using symbol_set = std::unordered_set<production::symbol>;
    virtual void parse(const std::vector<lexer::token>&) = 0;
    virtual void build() = 0;
    virtual ~grammar_base() = default;

    virtual void print_tree() const {
        tree_->print();
    }

    std::shared_ptr<tree> get_tree() const {
        return tree_;
    }

protected:
    std::vector<production::production> productions;
    std::unordered_map<production::symbol, symbol_set> first;
    std::unordered_map<production::symbol, symbol_set> follow;
    std::unordered_map<production::symbol, std::vector<std::size_t>> symbol_map;
    std::shared_ptr<tree> tree_ = std::make_shared<tree>();

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

    symbol_set calc_first(std::vector<production::symbol> symbols) {
        symbol_set result;
        for (const auto& sym : symbols) {
            auto first_set = calc_first(sym);
            result.insert(first_set.begin(), first_set.end());
            if (!first_set.count(production::symbol::epsilon)) {
                break;
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

        for (auto& _it_sym : follow) {
            auto& sym = std::get<0>(_it_sym);
            auto& follow_set = std::get<1>(_it_sym);
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

#endif// End include "grammar_base.hpp"
// Begin include "production.hpp"
// End include "production.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
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

std::ostream& operator<<(std::ostream& os, const action& act) {
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

std::ostream& operator<<(std::ostream& os, const LR_stack_t& stack) {
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

template <typename Production = production::LR_production>
class SLR : public grammar_base {
public:
    using production_t = Production;
    // using production_t = production::LR_production; // for intellisense
    static_assert(std::is_base_of<production::LR_production, production_t>::value);

    using action_table_t = std::unordered_map<std::size_t, std::unordered_map<production::symbol, action>>;
    using goto_table_t = std::unordered_map<std::size_t, std::unordered_map<production::symbol, std::size_t>>;
    using error_handle_fn = std::function<void(std::stack<LR_stack_t>&, std::vector<lexer::token>&, std::size_t&)>;

    explicit SLR(const std::vector<production::production>& productions_) {
        productions = productions_;

        const auto& first_prod = productions_[0];
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
    }

    void build() override {
        calc_first();
        calc_follow();
        build_items_set();
#ifdef SHOW_DEBUG
        print_items_set();
        print_tables();
#endif
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
            auto cur_input = production::symbol{in[pos]};
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
                    tree_->add_r(*it);
                }
                for (const auto& tk : in) {
                    tree_->update_r(production::symbol{tk});
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

    void init_error_handlers(std::function<void(action_table_t&, goto_table_t&, std::vector<error_handle_fn>&)> fn) {
        init_error_handlers_fn = std::move(fn);
    }

protected:
    using items_t = std::unordered_set<production_t>;

    std::vector<items_t> items_set;
    std::vector<symbol_set> after_dot_set;
    action_table_t action_table;
    goto_table_t goto_table;
    rightmost_step steps;

    std::vector<error_handle_fn> error_handlers;

    std::function<void(action_table_t&, goto_table_t&, std::vector<error_handle_fn>&)> init_error_handlers_fn;

    virtual void init_first_item_set() {
        items_t initial_items{production_t(productions[0])};
        add_closure(initial_items, 0);
    }

    void build_items_set() {
        items_set.reserve(productions.size() * 2);
        after_dot_set.reserve(productions.size() * 2);

        symbol_set after_dot;
        after_dot.insert(productions[0].rhs[0]);
        after_dot_set.emplace_back(std::move(after_dot));

        init_first_item_set();

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

        if (init_error_handlers_fn) {
            init_error_handlers_fn(action_table, goto_table, error_handlers);
        }

        // for (auto& [row, col] : action_table) {
        //     if (!col.count(production::symbol{"}"})) {
        //         col[production::symbol{"}"}] = action::error(0);
        //     }
        // }

        // error_handlers.emplace_back([&](std::stack<LR_stack_t>& stack, std::vector<lexer::token>& in, std::size_t& pos) {
        //     std::cerr << "Missing semicolon at line " << in[pos].line << ", column " << in[pos].column << std::endl;
        //     in.insert(in.begin() + pos, lexer::token{";"});
        //     steps.insert_symbol(in.size() - pos - 1, production::symbol{";"});
        // });
    }

    /**
     * 扩展给定的符号集：对于每个非终结符，查找其产生式，将对应的LR(0)项插入当前项集，
     * 并收集新出现的点后的符号（未被after_dot包含的），用于后续扩展。
     *
     * @param symbols 需要扩展的符号集，通常是当前项集点后的符号。
     * @param current_item_set 当前LR(0)项集，会被插入新的项。
     * @param after_dot 已经考虑过的点后符号集，用于去重。
     * @return 新增的点后符号集（未被考虑过的），用于下一轮扩展。
     */
    virtual symbol_set expand_item_set(const symbol_set& symbols, items_t& current_item_set, const symbol_set& after_dot) {
        symbol_set to_add;
        for (const auto& sym : symbols) {
            if (!sym.is_non_terminal()) {
                continue;
            }
            for (auto id : symbol_map[sym]) {
                auto lr_prod = production_t(productions[id]);
                current_item_set.insert(lr_prod);
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

    virtual void build_acc_and_reduce(const items_t& current_items, const std::size_t idx) {
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
    }

    /**
     * @brief 为给定的项集添加闭包到SLR分析器的项集集合中。
     *
     * 此函数针对指定索引（idx）在after_dot_set中的项集，计算其闭包。
     * 它会不断对项集应用闭包操作，直到没有新项可加入为止。
     * 如果生成的项集已存在于items_set中，则会删除after_dot_set中对应的条目，并返回已存在项集的索引。
     * 否则，将新项集加入items_set，并根据需要更新action_table（如accept或reduce动作）。
     *
     * @param current_items 当前需要扩展的项集的引用。
     * @param idx after_dot_set中对应项集的索引。
     * @return std::pair<bool, std::size_t>
     *         - 第一个元素为true表示找到新闭包或已存在闭包，false表示current_items为空。
     *         - 第二个元素为该项集在items_set中的索引，若未找到/添加则为-1。
     */
    virtual std::pair<bool, std::size_t> add_closure(items_t& current_items, std::size_t idx) {
        auto& after_dot = after_dot_set[idx];

        auto symbols = after_dot;
        bool is_empty;
        do {
            auto to_add = expand_item_set(symbols, current_items, after_dot);
            after_dot.insert(to_add.begin(), to_add.end());
            is_empty = to_add.empty();
            symbols = std::move(to_add);
        } while (!is_empty);

        if (current_items.empty()) {
            return {false, -1};
        }

        const auto res = std::find(items_set.begin(), items_set.end(), current_items);
        if (res != items_set.end()) {
            after_dot_set.erase(std::next(after_dot_set.begin(), idx));
            return {true, res - items_set.begin()};
        }

        build_acc_and_reduce(current_items, idx);

        items_set.emplace_back(std::move(current_items));
        return {true, items_set.size() - 1};
    }

    /**
     * @brief 在指定项集内，将点（dot）向指定符号 sym 后移，并生成新的项集。
     *
     * 该函数遍历索引为 idx 的项集中的所有项，查找点前为 sym 的项，将点向后移动一位，形成新的项集。
     * 对新项集中的每个项，如果点未到末尾，则收集点后的符号，形成 after_dot 集合。
     * 如果新项集非空，则为其添加闭包，并根据 sym 的类型（非终结符/终结符/结束符）更新 GOTO 或 ACTION 表。
     *
     * @param idx 当前项集在项集集合中的索引。
     * @param sym 要将点移过的文法符号。
     */
    virtual void move_dot(std::size_t idx, const production::symbol& sym) {
        const auto& items = items_set[idx];
        items_t new_items;
        symbol_set after_dot;
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
            auto _tmp = add_closure(new_items, after_dot_set.size() - 1);
            auto& success = std::get<0>(_tmp);
            auto& to = std::get<1>(_tmp);
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
        for (const auto& _it_state : action_table) {
            auto& state = std::get<0>(_it_state);
            auto& row = std::get<1>(_it_state);
            for (const auto& _it_sym : row) {
                auto& sym = std::get<0>(_it_sym);
                auto& act = std::get<1>(_it_sym);
                terminals.insert(sym);
            }
        }

        std::set<production::symbol> non_terminals;
        for (const auto& _it_state : goto_table) {
            auto& state = std::get<0>(_it_state);
            auto& row = std::get<1>(_it_state);
            for (const auto& _it_sym : row) {
                auto& sym = std::get<0>(_it_sym);
                auto& target = std::get<1>(_it_sym);
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
        for (const auto& _it_state : action_table) {
            auto& state = std::get<0>(_it_state);
            auto& _ = std::get<1>(_it_state);
            all_states.insert(state);
        }
        for (const auto& _it_state : goto_table) {
            auto& state = std::get<0>(_it_state);
            auto& _ = std::get<1>(_it_state);
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

#endif// End include "SLR.hpp"
// Begin include "grammar_base.hpp"
// End include "grammar_base.hpp"


#endif // GRAMMAR_GRAMMAR_HPP// End include "../grammar/grammar.hpp"
// Begin include "sema_production.hpp"
#ifndef SEMANTIC_SEMA_PRODUCTION_HPP
#define SEMANTIC_SEMA_PRODUCTION_HPP

// Begin include "../grammar/production.hpp"
// End include "../grammar/production.hpp"
// Begin include "../utils.hpp"
#ifndef UTILS_HPP
#define UTILS_HPP
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace utils {
std::string trim(const std::string& str) {
    const auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    const auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

#if __cplusplus >= 201703L
std::string_view trim(std::string_view str) {
    const auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string_view::npos) return "";
    const auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}
#endif

std::vector<std::string> split_trim(const std::string& s, const std::string& delimiter) {
    std::vector<std::string> result;
    size_t pos = 0, last_pos = 0;

    while ((pos = s.find(delimiter, last_pos)) != std::string::npos) {
        if (pos > last_pos) {
            auto sub = trim(s.substr(last_pos, pos - last_pos));
            if (!sub.empty()) {
                result.emplace_back(sub);
            }
        }
        last_pos = pos + delimiter.length();
    }

    if (last_pos < s.size()) {
        auto sub = trim(s.substr(last_pos, pos - last_pos));
        if (!sub.empty()) {
            result.emplace_back(sub);
        }
    }

    return result;
}

#if __cplusplus >= 201703L
std::vector<std::string_view> split_trim(std::string_view s, std::string_view delimiter) {
    std::vector<std::string_view> result;
    size_t pos = 0;

    while (true) {
        size_t next = s.find(delimiter, pos);
        if (next == std::string_view::npos) {
            if (pos < s.size()) {
                auto sub = trim(s.substr(pos));
                if (!sub.empty()) {
                    result.emplace_back(sub);
                }
            }
            break;
        }
        if (next > pos) {
            auto sub = trim(s.substr(pos, next - pos));
            if (!sub.empty()) {
                result.emplace_back(sub);
            }
        }
        pos = next + delimiter.size();
    }

    return result;
}
#endif

std::string join(const std::vector<std::string>& strings, const std::string& delimiter) {
    std::string result;
    for (const auto& s : strings) {
        if (!result.empty()) {
            result += delimiter;
        }
        result += s;
    }
    return result;
}

#if __cplusplus >= 201703L
std::string join(const std::vector<std::string_view>& strings, const std::string_view& delimiter) {
    std::string result;
    for (const auto& s : strings) {
        if (!result.empty()) {
            result += delimiter;
        }
        result += s;
    }
    return result;
}
#endif

template <typename T>
void print(std::ostream& os, const std::vector<T>& v) {
    for (const auto& elem : v) {
        os << elem << " ";
    }
}

template <typename K, typename V>
void print(std::ostream& os, const std::unordered_map<K, V>& m) {
    os << "{";
    for (auto it = m.begin(); it != m.end(); ++it) {
        os << it->first << "=" << it->second;
        if (std::next(it) != m.end()) {
            os << ",";
        }
    }
    os << "}";
}

template <typename T>
void print(T&& t) {
    print(std::cout, std::forward<T>(t));
}

template <typename T>
void println(std::ostream& os, T&& t) {
    print(os, std::forward<T>(t));
    os << '\n';
}

template <typename T>
void println(T&& t) {
    println(std::cout, std::forward<T>(t));
}

bool starts_with(const std::string& str, const std::string& prefix) {
#if __cplusplus >= 202002L
    return str.starts_with(prefix);
#else
    if (str.find(prefix) == 0) {
        return true;
    }
    return false;
#endif
}
} // namespace utils

#endif// End include "../utils.hpp"
#include <cassert>
#include <functional>
#include <memory>
#include <ostream>
#include <unordered_map>

namespace semantic {
class sema_symbol final : public grammar::production::symbol {
public:
    std::unordered_map<std::string, std::string> syn{};
    std::unordered_map<std::string, std::string> inh{};

    sema_symbol() = default;

    sema_symbol(const std::string& str) : grammar::production::symbol(str) {}

#if __cplusplus >= 201703L
    sema_symbol(const std::string_view str) : grammar::production::symbol(std::string(str)) {}
#endif

    sema_symbol(const char* str) : grammar::production::symbol(std::string(str)) {}

    friend std::ostream& operator<<(std::ostream& os, const sema_symbol& sym) {
        os << sym.name << "[" << "lexval=" << sym.lexval;
        if (!sym.syn.empty()) {
            os << ",syn=";
            utils::print(os, sym.syn);
        }
        if (!sym.inh.empty()) {
            os << ",inh=";
            utils::print(os, sym.inh);
        }
        os << "]";
        return os;
    }
};

class symbol_table {
public:
    using symbol_info = std::unordered_map<std::string, std::string>;

    void enter_scope_copy() {
        if (scopes.empty()) {
            scopes.emplace_back();
        } else {
            scopes.push_back(scopes.back());
            auto& current_scope = scopes.back();
            for (auto it = std::next(scopes.rbegin()); it != scopes.rend(); ++it) {
                for (auto& _it_key : *it) {
                    auto& key = std::get<0>(_it_key);
                    auto& value = std::get<1>(_it_key);
                    if (!current_scope.count(key)) {
                        current_scope[key] = value;
                    }
                }
            }
        }
    }

    void for_each_current(const std::function<void(const std::string&, const symbol_info&)>& func) const {
        if (!scopes.empty()) {
            const auto& current_scope = scopes.back();
            for (const auto& _it_key : current_scope) {
                auto& key = std::get<0>(_it_key);
                auto& value = std::get<1>(_it_key);
                func(key, value);
            }
        }
    }

    void enter_scope() {
        scopes.emplace_back();
    }

    void exit_scope() {
        assert(!scopes.empty() && "No scope to exit");
        scopes.pop_back();
    }

    bool insert(const std::string& name, const symbol_info& info) {
        if (scopes.empty()) {
            enter_scope();
        }
        auto& current_scope = scopes.back();
        if (current_scope.find(name) != current_scope.end()) {
            return false;
        }
        current_scope[name] = info;
        current_scope[name]["name"] = name;
        return true;
    }

    symbol_info* lookup(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto& scope = *it;
            auto found = scope.find(name);
            if (found != scope.end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unordered_map<std::string, symbol_info>> scopes;
};

class sema_env {
public:
    std::vector<std::string> errors;
    std::vector<std::unordered_map<std::string, std::shared_ptr<sema_symbol>>> symbols;

    symbol_table table;

    void error(const std::string& msg) {
        errors.emplace_back(msg);
    }

    sema_symbol& symbol(const std::string& name) {
        return *symbols.back()[name];
    }

    void enter_symbol_scope() {
        symbols.emplace_back();
    }

    void add_symbol(std::shared_ptr<sema_symbol> sym) {
        auto& back = symbols.back();
        if (!back.count(sym->name)) {
            back[sym->name] = sym;
            return;
        }
        int cnt = 1;
        for (const auto& _it_key : back) {
            auto& key = std::get<0>(_it_key);
            auto& _ = std::get<1>(_it_key);
            if (utils::starts_with(key, sym->name + "<")) {
                cnt++;
            }
        }
        back[sym->name + "<" + std::to_string(cnt) + ">"] = sym;
    }

    void exit_symbol_scope() {
        symbols.pop_back();
    }
};

class sema_production {
public:
    using symbol = semantic::sema_symbol;
    using action = std::function<void(sema_env& env)>;

    class rhs_value_t {
        union {
            symbol sym;
            action act;
        };

    public:
        bool is_symbol;
        bool is_action;

        rhs_value_t(const symbol& sym) : sym(sym), is_symbol(true), is_action(false) {}
        rhs_value_t(const action& act) : act(act), is_symbol(false), is_action(true) {}

        rhs_value_t(const char* str) : rhs_value_t(symbol(str)) {}

        symbol& get_symbol() {
            assert(is_symbol && "Attempting to get symbol from an action");
            return sym;
        }

        action& get_action() {
            assert(is_action && "Attempting to get action from a symbol");
            return act;
        }

        const symbol& get_symbol() const {
            assert(is_symbol && "Attempting to get symbol from an action");
            return sym;
        }

        const action& get_action() const {
            assert(is_action && "Attempting to get action from a symbol");
            return act;
        }

        friend std::ostream& operator<<(std::ostream& os, const rhs_value_t& val) {
            if (val.is_symbol) {
                os << val.sym;
            } else if (val.is_action) {
                os << "[action]";
            }
            return os;
        }

        rhs_value_t(const rhs_value_t& other) : is_symbol(other.is_symbol), is_action(other.is_action) {
            if (is_action) {
                new (&act) action(other.act);
            } else {
                new (&sym) symbol(other.sym);
            }
        }

        rhs_value_t& operator=(const rhs_value_t& other) {
            if (this != &other) {
                this->~rhs_value_t();
                new (this) rhs_value_t(other);
            }
            return *this;
        }

        ~rhs_value_t() {
            if (is_action) {
                act.~action();
            } else {
                sym.~symbol();
            }
        }
    };

    sema_symbol lhs;
    std::vector<rhs_value_t> rhs;

    explicit operator grammar::production::production() const {
        grammar::production::production prod;
        prod.lhs = this->lhs;
        for (const auto& r : this->rhs) {
            if (r.is_symbol) {
                prod.rhs.emplace_back(r.get_symbol());
            }
        }
        return prod;
    }

    sema_production() = default;

#ifdef SEMA_PROD_USE_INITIALIZER_LIST
    sema_production(const std::string& lhs_str, std::initializer_list<rhs_value_t> rhs_values)
        : lhs(lhs_str), rhs(rhs_values) {}
#else
#if __cplusplus >= 201703L
    template <typename... Args>
    sema_production(const std::string_view lhs_str, Args&&... rhs_values)
        : lhs(lhs_str), rhs{rhs_value_t(std::forward<Args>(rhs_values))...} {}
#else
    template <typename... Args>
    sema_production(const std::string& lhs_str, Args&&... rhs_values)
        : lhs(lhs_str), rhs{rhs_value_t(std::forward<Args>(rhs_values))...} {}
#endif
#endif
        
    sema_production replace(const grammar::production::symbol& sym) {
        auto new_prod = *this;
        for (auto& r : new_prod.rhs) {
            if (r.is_symbol && r.get_symbol().name == sym.name) {
                r.get_symbol().lexval = sym.lexval;
            }
        }
        return new_prod;
    }
};

#define ACT(...) semantic::sema_production::rhs_value_t([](semantic::sema_env& env) { __VA_ARGS__ })
#define GET(x) auto& x = env.symbol(#x)
#define TO_STRING(x) #x
#define GETI(x, i) auto& x##_##i = env.symbol(TO_STRING(x<i>))

} // namespace semantic

#endif// End include "sema_production.hpp"
// Begin include "sema_tree.hpp"
#include <cassert>
#include <memory>
#ifndef SEMANTIC_SEMA_TREE_HPP
#define SEMANTIC_SEMA_TREE_HPP

// Begin include "../grammar/tree.hpp"
// End include "../grammar/tree.hpp"
// Begin include "sema_production.hpp"
// End include "sema_production.hpp"
#include <unordered_map>
#include <vector>

namespace semantic {
struct sema_tree_node : public grammar::tree_node {
    using action_t = sema_production::action;
    using symbol_t = sema_symbol;

    action_t action = nullptr;

    bool is_action() const {
        return action != nullptr;
    }

    bool is_symbol() const {
        return symbol != nullptr;
    }

    explicit sema_tree_node(const symbol_t& symbol) : grammar::tree_node(std::make_shared<symbol_t>(symbol)) {}

    explicit sema_tree_node(const action_t& action) : grammar::tree_node(nullptr), action(action) {}

    explicit sema_tree_node(const sema_production::rhs_value_t& value) : grammar::tree_node(nullptr) {
        if (value.is_symbol) {
            symbol = std::make_shared<symbol_t>(value.get_symbol());
        } else if (value.is_action) {
            action = value.get_action();
        }
    }
};

class sema_tree : public grammar::tree {
    using production = grammar::production::production;

private:
    std::unordered_map<production, sema_production> prod_map;

public:
    sema_tree(const std::vector<sema_production>& productions) {
        for (const auto& prod : productions) {
            prod_map[production(prod)] = prod;
        }
    }

    virtual void add(const production& prod) override {
        const auto& sema_prod = prod_map[prod];
        if (!root) {
            root = std::make_shared<sema_tree_node>(sema_prod.lhs);
            std::vector<std::shared_ptr<grammar::production::symbol>> tmp;
            for (const auto& rhs : sema_prod.rhs) {
                auto node = std::make_shared<sema_tree_node>(rhs);
                if (rhs.is_symbol && rhs.get_symbol().is_terminal()) {
                    tmp.emplace_back(node->symbol);
                }
                node->parent = root;
                root->children.push_back(node);
                if (!next && rhs.is_symbol && rhs.get_symbol().is_non_terminal()) {
                    next = node;
                }
            }
            for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) {
                to_replace.push_back(*it);
            }
            for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
                const auto& sym = (*it)->symbol;
                if (sym && sym->is_non_terminal()) {
                    next_r = *it;
                    break;
                }
            }
            return;
        }
        assert(next != nullptr);
        assert(next->children.empty());
        assert(*next->symbol == sema_prod.lhs);
        bool found = false;
        std::shared_ptr<sema_tree_node> new_next = nullptr;
        std::vector<std::shared_ptr<grammar::production::symbol>> tmp;
        for (const auto& rhs : sema_prod.rhs) {
            auto node = std::make_shared<sema_tree_node>(rhs);
            if (rhs.is_symbol && rhs.get_symbol().is_terminal()) {
                tmp.emplace_back(node->symbol);
            }
            node->parent = next;
            next->children.push_back(node);
            if (!found && rhs.is_symbol && rhs.get_symbol().is_non_terminal()) {
                new_next = node;
                found = true;
            }
        }
        for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) {
            to_replace.push_back(*it);
        }
        if (found) {
            next = new_next;
        } else {
            next = next->parent;
            while (next) {
                for (const auto& child : next->children) {
                    if (child->symbol && child->symbol->is_non_terminal() && child->children.empty()) {
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

    virtual void add_r(const production& prod) override {
        const auto& sema_prod = prod_map[prod];
        if (!root) {
            root = std::make_shared<sema_tree_node>(sema_prod.lhs);
            for (const auto& rhs : sema_prod.rhs) {
                auto node = std::make_shared<sema_tree_node>(rhs);
                node->parent = root;
                root->children.push_back(node);
            }
            for (auto it = root->children.rbegin(); it != root->children.rend(); ++it) {
                auto snode = std::static_pointer_cast<sema_tree_node>(*it);
                if (snode->is_symbol() && snode->symbol->is_non_terminal()) {
                    next_r = snode;
                    break;
                }
            }
            return;
        }

        assert(next_r != nullptr);
        assert(next_r->children.empty());
        assert(*next_r->symbol == sema_prod.lhs);

        for (const auto& rhs : sema_prod.rhs) {
            auto node = std::make_shared<sema_tree_node>(rhs);
            node->parent = next_r;
            next_r->children.push_back(node);
        }

        bool found = false;

        for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
            auto snode = std::static_pointer_cast<sema_tree_node>(*it);
            if (snode->is_symbol() && snode->symbol->is_non_terminal()) {
                next_r = snode;
                found = true;
                break;
            }
        }

        if (!found) {
            next_r = next_r->parent;
            while (next_r) {
                for (auto it = next_r->children.rbegin(); it != next_r->children.rend(); ++it) {
                    auto snode = std::static_pointer_cast<sema_tree_node>(*it);
                    if (snode->is_symbol()
                        && snode->symbol->is_non_terminal()
                        && snode->children.empty()) {
                        next_r = snode;
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

    virtual void print_node(const std::shared_ptr<grammar::tree_node>& node, const int depth) const override {
        auto snode = std::static_pointer_cast<sema_tree_node>(node);
        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }
        if (snode->is_symbol()) {
            auto ssym = std::static_pointer_cast<sema_symbol>(snode->symbol);
            std::cout << *ssym << '\n';
        } else {
            std::cout << "[action]\n";
        }
        for (const auto& child : node->children) {
            print_node(child, depth + 1);
        }
    }

    sema_env calc() {
        sema_env env;
        calc_node(root, env);
        return env;
    }

private:
    void calc_node(std::shared_ptr<grammar::tree_node> node, sema_env& env) {
        auto snode = std::static_pointer_cast<sema_tree_node>(node);

        if (snode->is_action()) {
            snode->action(env);
            return;
        }
        if (snode->children.empty()) {
            return;
        }
        env.enter_symbol_scope();
        env.add_symbol(std::static_pointer_cast<sema_symbol>(snode->symbol));
        for (auto child : node->children) {
            auto schild = std::static_pointer_cast<sema_tree_node>(child);
            if (schild->is_symbol()) {
                env.add_symbol(std::static_pointer_cast<sema_symbol>(schild->symbol));
            }
        }
        for (auto child : node->children) {
            calc_node(child, env);
        }
        env.exit_symbol_scope();
    }
};
} // namespace semantic

#endif// End include "sema_tree.hpp"
#include <memory>
#include <type_traits>
#include <vector>

namespace semantic {
std::vector<grammar::production::production> to_productions(const std::vector<sema_production>& sema_prods) {
    std::vector<grammar::production::production> prods;
    prods.reserve(sema_prods.size());
    for (const auto& sema_prod : sema_prods) {
        prods.emplace_back(static_cast<grammar::production::production>(sema_prod));
    }
    return prods;
}

template <typename T>
class sema : public T {
public:
    static_assert(std::is_base_of<grammar::grammar_base, T>::value, "T must be a subclass of grammar::grammar_base");
    explicit sema(const std::vector<sema_production>& productions) : T(to_productions(productions)) {
        this->tree_ = std::make_shared<sema_tree>(productions);
    }
};
} // namespace semantic// End include "semantic/sema_grammar.hpp"
#include <memory>

enum class token_type {
    UNKNOWN = 0,
    INT,
    REAL,
    IF,
    THEN,
    ELSE,
    LPAR,
    RPAR,
    SEMI,
    LBRACE,
    RBRACE,
    PLUS,
    MINUS,
    MULT,
    DIV,
    LT,
    LE,
    GT,
    GE,
    EQ,
    ASSIGN,
    ID,
    INTNUM,
    REALNUM,
    WHITESPACE
};

const lexer::lexer::input_keywords_t<token_type> keywords = {
    {"int", token_type::INT, "int"},
    {"real", token_type::REAL, "real"},
    {"if", token_type::IF, "if"},
    {"then", token_type::THEN, "then"},
    {"else", token_type::ELSE, "else"},
    {"\\(", token_type::LPAR, "("},
    {"\\)", token_type::RPAR, ")"},
    {";", token_type::SEMI, ";"},
    {"\\{", token_type::LBRACE, "{"},
    {"\\}", token_type::RBRACE, "}"},
    {"\\+", token_type::PLUS, "+"},
    {"-", token_type::MINUS, "-"},
    {"\\*", token_type::MULT, "*"},
    {"/", token_type::DIV, "/"},
    {"<", token_type::LT, "<"},
    {"<=", token_type::LE, "<="},
    {">", token_type::GT, ">"},
    {">=", token_type::GE, ">="},
    {"==", token_type::EQ, "=="},
    {"=", token_type::ASSIGN, " ="},
    {"[a-zA-Z_][a-zA-Z0-9_]*", token_type::ID, "ID"},
    {"[0-9]+", token_type::INTNUM, "INTNUM"},
    {"[0-9]+\\.[0-9]*", token_type::REALNUM, "REALNUM"},
    {"[ \t\n]+", token_type::WHITESPACE, "WHITESPACE"}};

const std::unordered_set<std::string> terminals{
    "int", "real", "if", "then", "else", "(", ")", ";", "{", "}", "+", "-", "*",
    "/", "<", "<=", ">", ">=", "==", "=", "ID", "INTNUM", "REALNUM"};
std::vector<semantic::sema_production> build_grammar() {
    grammar::set_epsilon_str("E");
    grammar::set_end_mark_str("$");
    grammar::set_terminal_rule([&](const std::string& str) {
        return terminals.count(str);
    });
#ifdef SEMA_PROD_USE_INITIALIZER_LIST
    const std::vector<semantic::sema_production> prods{
        {"program", {"decls", "compoundstmt"}},
        {"decls", {"decl", ";", "decls"}},
        {"decls", {"E"}},
        {"decl", {"int", "ID", "=", "INTNUM", ACT(GET(ID); GET(INTNUM); env.table.insert(ID.lexval, {{"type", "int"}, {"value", INTNUM.lexval}});)}},
        {"decl", {"real", "ID", "=", "REALNUM", ACT(GET(ID); GET(REALNUM); env.table.insert(ID.lexval, {{"type", "real"}, {"value", REALNUM.lexval}});)}},
        {"stmt", {"ifstmt"}},
        {"stmt", {"assgstmt"}},
        {"stmt", {"compoundstmt"}},
        {"compoundstmt", {"{", ACT(env.table.enter_scope();), "stmts", "}", ACT(env.table.exit_scope();)}},
        {"stmts", {"stmt", "stmts"}},
        {"stmts", {"E"}},
        {"ifstmt", {"if", "(", "boolexpr", ")", "then", ACT(env.table.enter_scope_copy();), "stmt", ACT(GET(stmt); env.table.for_each_current([&](const std::string& key, const semantic::symbol_table::symbol_info& value) { stmt.inh[key] = value.at("value"); }); env.table.exit_scope();), "else", ACT(env.table.enter_scope_copy();), "stmt", ACT(GETI(stmt, 1); env.table.for_each_current([&](const std::string& key, const semantic::symbol_table::symbol_info& value) { stmt_1.inh[key] = value.at("value"); }); env.table.exit_scope();), ACT(GETI(stmt, 1); GET(stmt); GET(boolexpr); if (boolexpr.syn["val"] == "true") {
                for (auto& _it_key : stmt.inh) {
                    auto& key = std::get<0>(_it_key);
                    auto& value = std::get<1>(_it_key);
                    (*env.table.lookup(key))["value"] = value;
                } } else {
                for (auto& _it_key : stmt_1.inh) {
                    auto& key = std::get<0>(_it_key);
                    auto& value = std::get<1>(_it_key);
                    (*env.table.lookup(key))["value"] = value;
                } })}},
        {"assgstmt", {"ID", "=", "arithexpr", ";", ACT(GET(ID); GET(arithexpr); auto table_ID = env.table.lookup(ID.lexval); if (table_ID == nullptr) { env.error(ID.lexval + " is not defined"); return; }(*table_ID)["value"] = arithexpr.syn["val"];)}},
        {"boolexpr", {"arithexpr", "boolop", "arithexpr", ACT(GET(boolexpr); GET(boolop); GET(arithexpr); GETI(arithexpr, 1); auto lhs = std::stod(arithexpr.syn["val"]); auto rhs = std::stod(arithexpr_1.syn["val"]); if (boolop.syn["op"] == "<") { boolexpr.syn["val"] = lhs < rhs ? "true" : "false"; } if (boolop.syn["op"] == ">") { boolexpr.syn["val"] = lhs > rhs ? "true" : "false"; } if (boolop.syn["op"] == "==") { boolexpr.syn["val"] = lhs == rhs ? "true" : "false"; } if (boolop.syn["op"] == "<=") { boolexpr.syn["val"] = lhs <= rhs ? "true" : "false"; } if (boolop.syn["op"] == ">=") { boolexpr.syn["val"] = lhs >= rhs ? "true" : "false"; })}},
        {"boolop", {"<", ACT(GET(boolop); boolop.syn["op"] = "<";)}},
        {"boolop", {">", ACT(GET(boolop); boolop.syn["op"] = ">";)}},
        {"boolop", {"<=", ACT(GET(boolop); boolop.syn["op"] = "<=";)}},
        {"boolop", {">=", ACT(GET(boolop); boolop.syn["op"] = ">=";)}},
        {"boolop", {"==", ACT(GET(boolop); boolop.syn["op"] = "==";)}},
        {"arithexpr", {"multexpr", ACT(GET(arithexprprime); GET(multexpr); arithexprprime.inh["val"] = multexpr.syn["val"]; arithexprprime.inh["type"] = multexpr.syn["type"];), "arithexprprime", ACT(GET(arithexpr); GET(arithexprprime); arithexpr.syn["val"] = arithexprprime.syn["val"]; arithexpr.syn["type"] = arithexprprime.syn["type"];)}},
        {"arithexprprime", {"+", "multexpr", ACT(GET(arithexprprime); GET(multexpr); GETI(arithexprprime, 1); arithexprprime_1.inh["type"] = multexpr.syn["type"]; arithexprprime_1.inh["val"] = std::to_string(std::stod(multexpr.syn["val"]) + std::stod(arithexprprime.inh["val"]));), "arithexprprime", ACT(GET(arithexprprime); GETI(arithexprprime, 1); arithexprprime.syn["val"] = arithexprprime_1.syn["val"]; arithexprprime.syn["type"] = arithexprprime_1.syn["type"];)}},
        {"arithexprprime", {"-", "multexpr", ACT(GET(arithexprprime); GET(multexpr); GETI(arithexprprime, 1); arithexprprime_1.inh["type"] = multexpr.syn["type"]; arithexprprime_1.inh["val"] = std::to_string(std::stod(arithexprprime.inh["val"]) - std::stod(multexpr.syn["val"]));), "arithexprprime", ACT(GET(arithexprprime); GETI(arithexprprime, 1); arithexprprime.syn["val"] = arithexprprime_1.syn["val"]; arithexprprime.syn["type"] = arithexprprime_1.syn["type"];)}},
        {"arithexprprime", {"E", ACT(GET(arithexprprime); arithexprprime.syn["val"] = arithexprprime.inh["val"]; arithexprprime.syn["type"] = arithexprprime.inh["type"];)}},
        {"multexpr", {"simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); multexprprime.inh["val"] = simpleexpr.syn["val"]; multexprprime.inh["type"] = simpleexpr.syn["type"];), "multexprprime", ACT(GET(multexpr); GET(multexprprime); multexpr.syn["val"] = multexprprime.syn["val"]; multexpr.syn["type"] = multexprprime.syn["type"];)}},
        {"multexprprime", {"*", "simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); GETI(multexprprime, 1); multexprprime_1.inh["type"] = simpleexpr.syn["type"]; multexprprime_1.inh["val"] = std::to_string(std::stod(multexprprime.inh["val"]) * std::stod(simpleexpr.syn["val"]));), "multexprprime", ACT(GET(multexprprime); GETI(multexprprime, 1); multexprprime.syn["val"] = multexprprime_1.syn["val"]; multexprprime.syn["type"] = multexprprime_1.syn["type"];)}},
        {"multexprprime", {"/", "simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); GETI(multexprprime, 1); if (std::stod(simpleexpr.syn["val"]) == 0) {
                env.error("line " + std::to_string(simpleexpr.line) + ",division by zero");
                return; } multexprprime_1.inh["type"] = simpleexpr.syn["type"]; multexprprime_1.inh["val"] = std::to_string(std::stod(multexprprime.inh["val"]) / std::stod(simpleexpr.syn["val"]));), "multexprprime", ACT(GET(multexprprime); GETI(multexprprime, 1); multexprprime.syn["val"] = multexprprime_1.syn["val"]; multexprprime.syn["type"] = multexprprime_1.syn["type"];)}},
        {"multexprprime", {"E", ACT(GET(multexprprime); multexprprime.syn["val"] = multexprprime.inh["val"]; multexprprime.syn["type"] = multexprprime.inh["type"];)}},
        {"simpleexpr", {"ID", ACT(GET(simpleexpr); GET(ID); simpleexpr.syn["val"] = env.table.lookup(ID.lexval)->at("value"); simpleexpr.syn["type"] = env.table.lookup(ID.lexval)->at("type");)}},
        {"simpleexpr", {"INTNUM", ACT(GET(simpleexpr); GET(INTNUM); simpleexpr.syn["val"] = INTNUM.lexval; simpleexpr.syn["type"] = "int"; simpleexpr.update_pos(INTNUM);)}},
        {"simpleexpr", {"REALNUM", ACT(GET(simpleexpr); GET(REALNUM); simpleexpr.syn["val"] = REALNUM.lexval; simpleexpr.syn["type"] = "real";)}},
        {"simpleexpr", {"(", "arithexpr", ")", ACT(GET(simpleexpr); GET(arithexpr); simpleexpr.syn["val"] = arithexpr.syn["val"]; simpleexpr.syn["type"] = arithexpr.syn["type"];)}},
    };
#else
    const std::vector<semantic::sema_production> prods{
        {"program", "decls", "compoundstmt"},
        {"decls", "decl", ";", "decls"},
        {"decls", "E"},
        {"decl", "int", "ID", "=", "INTNUM",
         ACT(GET(ID); GET(INTNUM);
             env.table.insert(ID.lexval, {{"type", "int"}, {"value", INTNUM.lexval}});)},
        {"decl", "real", "ID", "=", "REALNUM",
         ACT(GET(ID); GET(REALNUM);
             env.table.insert(ID.lexval, {{"type", "real"}, {"value", REALNUM.lexval}});)},
        {"stmt", "ifstmt"},
        {"stmt", "assgstmt"},
        {"stmt", "compoundstmt"},
        {"compoundstmt", "{", ACT(env.table.enter_scope();), "stmts", "}", ACT(env.table.exit_scope();)},
        {"stmts", "stmt", "stmts"},
        {"stmts", "E"},
        {"ifstmt", "if", "(", "boolexpr", ")", "then",
         ACT(env.table.enter_scope_copy();),
         "stmt",
         ACT(GET(stmt);
             env.table.for_each_current([&](const std::string& key, const semantic::symbol_table::symbol_info& value) { stmt.inh[key] = value.at("value"); });
             env.table.exit_scope();),
         "else",
         ACT(env.table.enter_scope_copy();),
         "stmt",
         ACT(GETI(stmt, 1);
             env.table.for_each_current([&](const std::string& key, const semantic::symbol_table::symbol_info& value) { stmt_1.inh[key] = value.at("value"); });
             env.table.exit_scope();),
         ACT(GETI(stmt, 1); GET(stmt); GET(boolexpr); if (boolexpr.syn["val"] == "true") {
                for (auto& _it_key : stmt.inh) {
                    auto& key = std::get<0>(_it_key);
                    auto& value = std::get<1>(_it_key);
                    (*env.table.lookup(key))["value"] = value;
                } } else {
                for (auto& _it_key : stmt_1.inh) {
                    auto& key = std::get<0>(_it_key);
                    auto& value = std::get<1>(_it_key);
                    (*env.table.lookup(key))["value"] = value;
                } })},
        {"assgstmt", "ID", "=", "arithexpr", ";", ACT(GET(ID); GET(arithexpr); auto table_ID = env.table.lookup(ID.lexval); if (table_ID == nullptr) { env.error(ID.lexval + " is not defined"); return; }(*table_ID)["value"] = arithexpr.syn["val"];)},
        {"boolexpr", "arithexpr", "boolop", "arithexpr", ACT(GET(boolexpr); GET(boolop); GET(arithexpr); GETI(arithexpr, 1); auto lhs = std::stod(arithexpr.syn["val"]); auto rhs = std::stod(arithexpr_1.syn["val"]); if (boolop.syn["op"] == "<") { boolexpr.syn["val"] = lhs < rhs ? "true" : "false"; } if (boolop.syn["op"] == ">") { boolexpr.syn["val"] = lhs > rhs ? "true" : "false"; } if (boolop.syn["op"] == "==") { boolexpr.syn["val"] = lhs == rhs ? "true" : "false"; } if (boolop.syn["op"] == "<=") { boolexpr.syn["val"] = lhs <= rhs ? "true" : "false"; } if (boolop.syn["op"] == ">=") { boolexpr.syn["val"] = lhs >= rhs ? "true" : "false"; })},
        {"boolop", "<", ACT(GET(boolop); boolop.syn["op"] = "<";)},
        {"boolop", ">", ACT(GET(boolop); boolop.syn["op"] = ">";)},
        {"boolop", "<=", ACT(GET(boolop); boolop.syn["op"] = "<=";)},
        {"boolop", ">=", ACT(GET(boolop); boolop.syn["op"] = ">=";)},
        {"boolop", "==", ACT(GET(boolop); boolop.syn["op"] = "==";)},
        {"arithexpr", "multexpr", ACT(GET(arithexprprime); GET(multexpr); arithexprprime.inh["val"] = multexpr.syn["val"]; arithexprprime.inh["type"] = multexpr.syn["type"];), "arithexprprime", ACT(GET(arithexpr); GET(arithexprprime); arithexpr.syn["val"] = arithexprprime.syn["val"]; arithexpr.syn["type"] = arithexprprime.syn["type"];)},
        {"arithexprprime", "+", "multexpr", ACT(GET(arithexprprime); GET(multexpr); GETI(arithexprprime, 1); arithexprprime_1.inh["type"] = multexpr.syn["type"]; arithexprprime_1.inh["val"] = std::to_string(std::stod(multexpr.syn["val"]) + std::stod(arithexprprime.inh["val"]));), "arithexprprime", ACT(GET(arithexprprime); GETI(arithexprprime, 1); arithexprprime.syn["val"] = arithexprprime_1.syn["val"]; arithexprprime.syn["type"] = arithexprprime_1.syn["type"];)},
        {"arithexprprime", "-", "multexpr", ACT(GET(arithexprprime); GET(multexpr); GETI(arithexprprime, 1); arithexprprime_1.inh["type"] = multexpr.syn["type"]; arithexprprime_1.inh["val"] = std::to_string(std::stod(arithexprprime.inh["val"]) - std::stod(multexpr.syn["val"]));), "arithexprprime", ACT(GET(arithexprprime); GETI(arithexprprime, 1); arithexprprime.syn["val"] = arithexprprime_1.syn["val"]; arithexprprime.syn["type"] = arithexprprime_1.syn["type"];)},
        {"arithexprprime", "E", ACT(GET(arithexprprime); arithexprprime.syn["val"] = arithexprprime.inh["val"]; arithexprprime.syn["type"] = arithexprprime.inh["type"];)},
        {"multexpr", "simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); multexprprime.inh["val"] = simpleexpr.syn["val"]; multexprprime.inh["type"] = simpleexpr.syn["type"];), "multexprprime", ACT(GET(multexpr); GET(multexprprime); multexpr.syn["val"] = multexprprime.syn["val"]; multexpr.syn["type"] = multexprprime.syn["type"];)},
        {"multexprprime", "*", "simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); GETI(multexprprime, 1); multexprprime_1.inh["type"] = simpleexpr.syn["type"]; multexprprime_1.inh["val"] = std::to_string(std::stod(multexprprime.inh["val"]) * std::stod(simpleexpr.syn["val"]));), "multexprprime", ACT(GET(multexprprime); GETI(multexprprime, 1); multexprprime.syn["val"] = multexprprime_1.syn["val"]; multexprprime.syn["type"] = multexprprime_1.syn["type"];)},
        {"multexprprime", "/", "simpleexpr", ACT(GET(multexprprime); GET(simpleexpr); GETI(multexprprime, 1); if (std::stod(simpleexpr.syn["val"]) == 0) {
                env.error("line " + std::to_string(simpleexpr.line) + ",division by zero");
                return; } multexprprime_1.inh["type"] = simpleexpr.syn["type"]; multexprprime_1.inh["val"] = std::to_string(std::stod(multexprprime.inh["val"]) / std::stod(simpleexpr.syn["val"]));), "multexprprime", ACT(GET(multexprprime); GETI(multexprprime, 1); multexprprime.syn["val"] = multexprprime_1.syn["val"]; multexprprime.syn["type"] = multexprprime_1.syn["type"];)},
        {"multexprprime", "E", ACT(GET(multexprprime); multexprprime.syn["val"] = multexprprime.inh["val"]; multexprprime.syn["type"] = multexprprime.inh["type"];)},
        {"simpleexpr", "ID", ACT(GET(simpleexpr); GET(ID); simpleexpr.syn["val"] = env.table.lookup(ID.lexval)->at("value"); simpleexpr.syn["type"] = env.table.lookup(ID.lexval)->at("type");)},
        {"simpleexpr", "INTNUM", ACT(GET(simpleexpr); GET(INTNUM); simpleexpr.syn["val"] = INTNUM.lexval; simpleexpr.syn["type"] = "int"; simpleexpr.update_pos(INTNUM);)},
        {"simpleexpr", "REALNUM", ACT(GET(simpleexpr); GET(REALNUM); simpleexpr.syn["val"] = REALNUM.lexval; simpleexpr.syn["type"] = "real";)},
        {"simpleexpr", "(", "arithexpr", ")", ACT(GET(simpleexpr); GET(arithexpr); simpleexpr.syn["val"] = arithexpr.syn["val"]; simpleexpr.syn["type"] = arithexpr.syn["type"];)},
    };
#endif
    return prods;
};

std::string trim_zero(std::string s) {
    auto pos = s.find('.');
    if (pos == std::string::npos) {
        return s;
    }
    while (!s.empty() && s.back() == '0') {
        s.pop_back();
    }
    if (!s.empty() && s.back() == '.') {
        s.pop_back();
    }
    return s;
}

void Analysis()
{
    string prog;
    read_prog(prog);
    /* 骚年们 请开始你们的表演 */
    /********* Begin *********/
    // std::cout << prog << std::endl;
    const auto lex = lexer::lexer(keywords, token_type::WHITESPACE);
    const auto tokens = lex.parse(prog);
    using gram_t = grammar::SLR<>;
    auto gram = semantic::sema<gram_t>(build_grammar());
    gram.init_error_handlers([](gram_t::action_table_t& action_table, gram_t::goto_table_t& goto_table, std::vector<gram_t::error_handle_fn>& error_handlers) {
        const auto realnum = grammar::production::symbol{"REALNUM"};
        for (auto& _it_state : action_table) {
            auto& state = std::get<0>(_it_state);
            auto& row = std::get<1>(_it_state);
            if (!row.count(realnum)) {
                row[realnum] = grammar::action::error(0);
            }
        }

        error_handlers.emplace_back([](std::stack<grammar::LR_stack_t>& stack, std::vector<lexer::token>& in, std::size_t& pos) {
            std::cout << "error message:line " << in[pos].line << ",realnum can not be translated into int type" << std::endl;
            in[pos].type = static_cast<int>(token_type::INTNUM);
        });
    });
    gram.build();
    gram.parse(tokens);

    auto tree = std::static_pointer_cast<semantic::sema_tree>(gram.get_tree());

    // tree->print();
    auto env = tree->calc();
    // tree->print();

    for (const auto& error : env.errors) {
        std::cout << "error message:" << error << '\n';
    }

    if (!env.errors.empty()) {
        return;
    }

    std::vector<std::string> res;
    env.table.for_each_current([&](auto&& k, auto&& v) {
        res.push_back(k + ": " + trim_zero(v.at("value")));
    });

    std::sort(res.begin(), res.end());
    for (const auto& r : res) {
        std::cout << r << '\n';
    }
    /********* End *********/
    
}