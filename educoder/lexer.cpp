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


#ifndef REGEX_EXCEPTION_HPP

#include <exception>
#include <string>

namespace regex {
class unknown_character_exception : public std::exception {
public:
    explicit unknown_character_exception(std::string ch)
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

class invalid_regex_exception : public std::exception {
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

#endif // REGEX_EXCEPTION_HPP

#ifndef REGEX_TOKEN_HPP
#define REGEX_TOKEN_HPP

#include <initializer_list>
#include <iostream>
#include <ostream>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifndef FALLBACK_TOKEN_TYPE_HPP
#define FALLBACK_TOKEN_TYPE_HPP

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

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
    blackslash
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
    {op::blackslash, "\\"}};

enum class nothing {};

struct char_set {
    std::unordered_set<char> chars;
    bool is_negative = false;

    char_set() = default;
    explicit char_set(char from, char to, bool is_negative = false) : is_negative(is_negative) {
        for (char ch = from; ch <= to; ++ch) {
            chars.insert(ch);
        }
    }

    explicit char_set(std::initializer_list<char> init, bool is_negative = false) : is_negative(is_negative) {
        for (const auto& ch : init) {
            chars.insert(ch);
        }
    }

    explicit char_set(std::initializer_list<std::pair<char, char>> init, bool is_negative = false) : is_negative(is_negative) {
        for (const auto& p : init) {
            const auto& from = p.first;
            const auto& to = p.second;
            add(from, to);
        }
    }

    void add(char ch) {
        chars.insert(ch);
    }

    void add(const char_set& other) {
        chars.insert(other.chars.begin(), other.chars.end());
    }

    void add(char from, char to) {
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

namespace fallback {

using namespace regex::token;

template <typename T>
struct token_type_traits;

template <>
struct token_type_traits<nothing> {
    static constexpr token_type::kind value = token_type::kind::nothing;
};

template <>
struct token_type_traits<symbol> {
    static constexpr token_type::kind value = token_type::kind::symbol;
};

template <>
struct token_type_traits<op> {
    static constexpr token_type::kind value = token_type::kind::op;
};

template <>
struct token_type_traits<char_set> {
    static constexpr token_type::kind value = token_type::kind::char_set;
};

template <>
struct token_type_traits<char> {
    static constexpr token_type::kind value = token_type::kind::character;
};

template <typename T>
bool holds_alternative(const token_type& t) {
    return t.type == token_type_traits<T>::value;
}

template <typename T>
T& get(token_type& t) {
    if (!holds_alternative<T>(t)) {
        throw std::bad_cast();
    }

    if (std::is_same<T, nothing>::value) {
        return (T&)t.value._nothing;
    } else if (std::is_same<T, symbol>::value) {
        return (T&)t.value.sym;
    } else if (std::is_same<T, op>::value) {
        return (T&)t.value.operation;
    } else if (std::is_same<T, char_set>::value) {
        return (T&)t.value.set;
    } else if (std::is_same<T, char>::value) {
        return (T&)t.value.character;
    }
}

template <typename T>
const T& get(const token_type& t) {
    return get<T>(const_cast<token_type&>(t));
}

template <typename Visitor>
auto visit(Visitor&& vis, token_type& t) -> decltype(auto) {
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
auto visit(Visitor&& vis, const token_type& t) -> decltype(auto) {
    return visit(std::forward<Visitor>(vis), const_cast<token_type&>(t));
}

template <typename T>
T* get_if(token_type* t) {
    if (!t || !holds_alternative<T>(*t)) return nullptr;

    if (std::is_same<T, nothing>::value) {
        return (T*)&t->value._nothing;
    } else if (std::is_same<T, symbol>::value) {
        return (T*)&t->value.sym;
    } else if (std::is_same<T, op>::value) {
        return (T*)&t->value.operation;
    } else if (std::is_same<T, char_set>::value) {
        return (T*)&t->value.set;
    } else if (std::is_same<T, char>::value) {
        return (T*)&t->value.character;
    }
}

template <typename T>
const T* get_if(const token_type* t) {
    return get_if<T>(const_cast<token_type*>(t));
}

} // namespace fallback

#endif // FALLBACK_TOKEN_TYPE_HPP

namespace regex::token {

struct token_type_hash {
    std::size_t operator()(const token_type& t) const {
        if (fallback::holds_alternative<char>(t)) {
            return std::hash<char>{}(fallback::get<char>(t));
        } else if (fallback::holds_alternative<symbol>(t)) {
            return std::hash<int>{}(static_cast<int>(fallback::get<symbol>(t)));
        } else if (fallback::holds_alternative<op>(t)) {
            return std::hash<int>{}(static_cast<int>(fallback::get<op>(t)));
        } else if (fallback::holds_alternative<char_set>(t)) {
            std::size_t hash = 0;
            const auto& set = fallback::get<char_set>(t);
            for (const auto& ch : set.chars) {
                hash ^= std::hash<char>{}(ch);
            }
            return hash;
        }
        return 0;
    }
};

static int get_precedence(op opr) {
    switch (opr) {
    case op::star: return 3;
    case op::plus: return 3;
    case op::concat: return 2;
    case op::alt: return 1;
    case op::left_par: return 0;
    case op::right_par: return 0;
    case op::blackslash: return -1;
    }
    return -1;
}

static int get_precedence(token_type ch) {
    op opr;
    if (fallback::holds_alternative<op>(ch)) {
        opr = fallback::get<op>(ch);
    } else {
        return -1;
    }

    return get_precedence(opr);
}

static bool is_char(token_type ch) {
    return fallback::holds_alternative<char>(ch);
}

static bool is_symbol(token_type ch) {
    return fallback::holds_alternative<symbol>(ch);
}

static bool is_op(token_type ch) {
    return fallback::holds_alternative<op>(ch);
}

static bool is_char_set(token_type ch) {
    return fallback::holds_alternative<char_set>(ch);
}

template <typename T>
static bool is(token_type ch, T other) {
    if (auto* p = fallback::get_if<T>(&ch)) {
        return *p == other;
    }
    return false;
}

static bool match(char c, token_type ch) {
    if (is_char(ch)) {
        return fallback::get<char>(ch) == c;
    } else if (is_char_set(ch)) {
        const auto& set = fallback::get<char_set>(ch);
        if (set.is_negative) {
            return set.chars.find(c) == set.chars.end();
        } else {
            return set.chars.find(c) != set.chars.end();
        }
    }
    return false;
}

static bool is_nonop(char ch) {
    return ch != '\\' && ch != '|' && ch != '*' && ch != '(' && ch != ')' && ch != '+' && ch != '[' && ch != ']';
}

static bool is_nonop(token_type ch) {
    return is_char(ch) || is_symbol(ch) || is_char_set(ch);
}

static std::vector<token_type> split(const std::string& s) {
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

        if (is(last, op::blackslash)) {
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
                last = op::blackslash;
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

static std::vector<token_type> to_postfix(const std::vector<token_type>& v) {
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
            throw regex::unknown_character_exception(std::string{fallback::get<char>(ch)});
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

inline std::ostream& operator<<(std::ostream& os, const token_type& ch) {
    if (is_char(ch)) {
        char c = fallback::get<char>(ch);
        if (escape_map.find(c) != escape_map.end()) {
            os << escape_map.at(c);
        } else if (std::isprint(c)) {
            os << c;
        } else {
            os << '\\' << static_cast<int>(c);
        }
    } else if (is_op(ch)) {
        os << op_map.at(fallback::get<op>(ch));
    } else if (is_symbol(ch)) {
        os << symbol_map.at(fallback::get<symbol>(ch));
    } else if (is_char_set(ch)) {
        const auto& set = fallback::get<char_set>(ch);
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

// static void print(const std::vector<token_type>& v) {
//     for (const auto& ch : v) {
//         std::cout << ch << ' ';
//     }
//     std::cout << std::endl;
// }

static char_set to_char_set(const token_type& token) {
    if (fallback::holds_alternative<char>(token)) {
        return char_set{fallback::get<char>(token)};
    } else if (fallback::holds_alternative<char_set>(token)) {
        return fallback::get<char_set>(token);
    } else {
        throw std::invalid_argument("Invalid token type for conversion to char_set");
    }
}
} // namespace regex::token

#endif // REGEX_TOKEN_HPP


#ifndef REGEX_TREE_HPP
#define REGEX_TREE_HPP

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

class char_node : public regex_node {
public:
    token::token_type value;
    size_t number;

    explicit char_node(token::token_type ch, size_t number) : value(ch), number(number) {
        type = type::ch;
        nullable = false;
        firstpos.insert(number);
        lastpos.insert(number);
    }
};

class concat_node : public regex_node {
public:
    node_ptr_t left, right;
    explicit concat_node(node_ptr_t left, node_ptr_t right) : left(left), right(right) {
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

class star_node : public regex_node {
public:
    node_ptr_t child;
    explicit star_node(node_ptr_t child) : child(child) {
        type = type::star;
        nullable = true;
        firstpos = child->firstpos;
        lastpos = child->lastpos;
    }
};

class plus_node : public regex_node {
public:
    node_ptr_t child;
    explicit plus_node(node_ptr_t child) : child(child) {
        type = type::plus;
        nullable = child->nullable;
        firstpos = child->firstpos;
        lastpos = child->lastpos;
    }
};

class alt_node : public regex_node {
public:
    node_ptr_t left, right;
    explicit alt_node(node_ptr_t left, node_ptr_t right) : left(left), right(right) {
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
                for (auto i : concat_node.left->lastpos) {
                    followpos[i].insert(concat_node.right->firstpos.begin(), concat_node.right->firstpos.end());
                }
            } else if (node.type == regex_node::type::star) {
                auto& star_node = node.as<tree::star_node>();
                for (auto i : star_node.lastpos) {
                    followpos[i].insert(star_node.firstpos.begin(), star_node.firstpos.end());
                }
            } else if (node.type == regex_node::type::plus) {
                auto& plus_node = node.as<tree::plus_node>();
                for (auto i : plus_node.lastpos) {
                    followpos[i].insert(plus_node.firstpos.begin(), plus_node.firstpos.end());
                }
            }
        });

        token_map = disjoint_token_sets(token_map);
    }

    void visit(std::function<void(regex_node&)> func) {
        visit(root, func);
    }

    void print() const {
        std::cout << "Regex Tree" << std::endl;
        print(root);
    }

private:
    void visit(regex_node::node_ptr_t node, std::function<void(regex_node&)> func) {
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

    void print(regex_node::node_ptr_t node, int indent = 0) const {
        using token::op;

        if (node == nullptr) {
            return;
        }
        std::cout << std::string(indent, ' ');
        if (node->type == regex_node::type::ch) {
            auto& char_node = node->as<tree::char_node>();
            std::cout << char_node.value << '(' << char_node.number << ')' << std::endl;
        } else if (node->type == regex_node::type::concat) {
            std::cout << token::op_map.at(op::concat) << std::endl;
            print(node->as<tree::concat_node>().left, indent + 2);
            print(node->as<tree::concat_node>().right, indent + 2);
        } else if (node->type == regex_node::type::alt) {
            std::cout << token::op_map.at(op::alt) << std::endl;
            print(node->as<tree::alt_node>().left, indent + 2);
            print(node->as<tree::alt_node>().right, indent + 2);
        } else if (node->type == regex_node::type::star) {
            std::cout << token::op_map.at(op::star) << std::endl;
            print(node->as<tree::star_node>().child, indent + 2);
        } else if (node->type == regex_node::type::plus) {
            std::cout << token::op_map.at(op::plus) << std::endl;
            print(node->as<tree::plus_node>().child, indent + 2);
        }
    }

    struct unordered_set_hash {
        std::size_t operator()(const std::unordered_set<std::size_t>& s) const {
            std::size_t h = 0;
            for (auto v : s) {
                h ^= std::hash<std::size_t>{}(v);
            }
            return h;
        }
    };

    token_map_t disjoint_token_sets(const token_map_t& original) {
        std::unordered_map<char, std::unordered_set<size_t>> char_to_positions;

        for (const auto& p : original) {
            const auto& token = p.first;
            const auto& positions = p.second;
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
        for (const auto& p : char_to_positions) {
            const auto& ch = p.first;
            const auto& pos = p.second;
            grouped[pos].insert(ch);
        }

        token_map_t result;

        for (const auto& p : grouped) {
            const auto& positions = p.first;
            const auto& chars = p.second;
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


#ifndef REGEX_DFA_HPP
#define REGEX_DFA_HPP


#include <cstddef>
#include <unordered_set>

namespace regex::dfa {
class dfa {
public:
    using state_t = std::size_t;
    using token_t = regex::token::token_type;
    using token_t_hash = regex::token::token_type_hash;
    using transition_t = std::unordered_map<token_t, state_t, token_t_hash>;
    using dfa_state_t = std::unordered_map<state_t, transition_t>;

    dfa() = delete;

    explicit dfa(const regex::tree::regex_tree& tree) {
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

    void add_transition(state_t from, const token_t& token, state_t to) {
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
            for (const auto& p : transitions.at(current_state)) {
                const auto& token = p.first;
                const auto& to = p.second;
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
                for (const auto& p : transitions.at(current_state)) {
                    const auto& token = p.first;
                    const auto& to = p.second;
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
        for (const auto& p : transitions) {
            const auto& from = p.first;
            const auto& trans = p.second;
            for (const auto& pp : trans) {
                const auto& token = pp.first;
                const auto& to = pp.second;
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

#endif // REGEX_DFA_HPP

#ifndef REGEX_REGEX_HPP
#define REGEX_REGEX_HPP

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

#endif // REGEX_REGEX_HPP

#ifndef LEXER_KEY_WORDS_HPP
#define LEXER_KEY_WORDS_HPP

#include <utility>
#include <vector>

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

using key_word_t = std::pair<regex::regex, token_type>;

static const std::vector<key_word_t> key_words = {
    {regex::regex("auto"), AUTO},
    {regex::regex("break"), BREAK},
    {regex::regex("case"), CASE},
    {regex::regex("char"), CHAR},
    {regex::regex("const"), CONST},
    {regex::regex("continue"), CONTINUE},
    {regex::regex("default"), DEFAULT},
    {regex::regex("do"), DO},
    {regex::regex("double"), DOUBLE},
    {regex::regex("else"), ELSE},
    {regex::regex("enum"), ENUM},
    {regex::regex("extern"), EXTERN},
    {regex::regex("float"), FLOAT},
    {regex::regex("for"), FOR},
    {regex::regex("goto"), GOTO},
    {regex::regex("if"), IF},
    {regex::regex("int"), INT},
    {regex::regex("long"), LONG},
    {regex::regex("register"), REGISTER},
    {regex::regex("return"), RETURN},
    {regex::regex("short"), SHORT},
    {regex::regex("signed"), SIGNED},
    {regex::regex("sizeof"), SIZEOF},
    {regex::regex("static"), STATIC},
    {regex::regex("struct"), STRUCT},
    {regex::regex("switch"), SWITCH},
    {regex::regex("typedef"), TYPEDEF},
    {regex::regex("union"), UNION},
    {regex::regex("unsigned"), UNSIGNED},
    {regex::regex("void"), VOID},
    {regex::regex("volatile"), VOLATILE},
    {regex::regex("while"), WHILE},
    {regex::regex("-"), MINUS},
    {regex::regex("--"), MINUS_MINUS},
    {regex::regex("-="), MINUS_EQUAL},
    {regex::regex("->"), ARROW},
    {regex::regex("!"), BANG},
    {regex::regex("!="), BANG_EQUAL},
    {regex::regex("%"), PERCENT},
    {regex::regex("%="), PERCENT_EQUAL},
    {regex::regex("&"), AMPERSAND},
    {regex::regex("&&"), AMPERSAND_AMPERSAND},
    {regex::regex("&="), AMPERSAND_EQUAL},
    {regex::regex("\\("), LPAREN},
    {regex::regex("\\)"), RPAREN},
    {regex::regex("\\*"), ASTERISK},
    {regex::regex("\\*="), ASTERISK_EQUAL},
    {regex::regex(","), COMMA},
    {regex::regex("."), DOT},
    {regex::regex("/"), SLASH},
    {regex::regex("/="), SLASH_EQUAL},
    {regex::regex(":"), COLON},
    {regex::regex(";"), SEMICOLON},
    {regex::regex("?"), QUESTION},
    {regex::regex("\\["), LBRACKET},
    {regex::regex("\\]"), RBRACKET},
    {regex::regex("^"), CARET},
    {regex::regex("^="), CARET_EQUAL},
    {regex::regex("{"), LBRACE},
    {regex::regex("\\|"), PIPE},
    {regex::regex("\\|\\|"), PIPE_PIPE},
    {regex::regex("\\|="), PIPE_EQUAL},
    {regex::regex("}"), RBRACE},
    {regex::regex("~"), TILDE},
    {regex::regex("\\+"), PLUS},
    {regex::regex("\\+\\+"), PLUS_PLUS},
    {regex::regex("\\+="), PLUS_EQUAL},
    {regex::regex("<"), LESS},
    {regex::regex("<<"), LESS_LESS},
    {regex::regex("<<="), LESS_LESS_EQUAL},
    {regex::regex("<="), LESS_EQUAL},
    {regex::regex("="), EQUAL},
    {regex::regex("=="), EQUAL_EQUAL},
    {regex::regex(">"), GREATER},
    {regex::regex(">="), GREATER_EQUAL},
    {regex::regex(">>"), GREATER_GREATER},
    {regex::regex(">>="), GREATER_GREATER_EQUAL},
    {regex::regex("\""), QUOTE},
    {regex::regex("(//[^\n]*)|(/\\*([^*]|\\*+[^*/])*\\*+/)"), COMMENT},
    {regex::regex("[0-9]+"), NUMBER},
    {regex::regex("[a-zA-Z_%][a-zA-Z0-9_]*"), IDENTIFIER},
    {regex::regex("[ \t\r\n]+"), WHITESPACE}};
} // namespace lexer::token

#endif // LEXER_KEY_WORDS_HPP

#ifndef LEXER_LEXER_HPP
#define LEXER_LEXER_HPP

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
            for (const auto& p : token::key_words) {
                const auto& pattern = p.first;
                const auto& token = p.second;
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

void Analysis()
{
	string prog;
	read_prog(prog);
	/* 骚年们 请开始你们的表演 */
    /********* Begin *********/
    auto lex = lexer::lexer();

    auto res = lex.parse(prog);

    for (int i = 0; i < res.size(); ++i) {
        std::cout << i + 1 << ": <" << res[i].first << ',' << res[i].second << ">\n";
    }
    
    /********* End *********/
	
}

int main() {
    Analysis();
    return 0;
}