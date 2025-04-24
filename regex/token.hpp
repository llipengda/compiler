#pragma once

#ifndef REGEX_TOKEN_HPP
#define REGEX_TOKEN_HPP

#include "exception.hpp"

#include <initializer_list>
#include <iostream>
#include <ostream>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

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
        for (const auto& [from, to] : init) {
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

using token_type = std::variant<nothing, symbol, op, char_set, char>;

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
    if (std::holds_alternative<op>(ch)) {
        opr = std::get<op>(ch);
    } else {
        return -1;
    }

    return get_precedence(opr);
}

static bool is_char(token_type ch) {
    return std::holds_alternative<char>(ch);
}

static bool is_symbol(token_type ch) {
    return std::holds_alternative<symbol>(ch);
}

static bool is_op(token_type ch) {
    return std::holds_alternative<op>(ch);
}

static bool is_char_set(token_type ch) {
    return std::holds_alternative<char_set>(ch);
}

template <typename T>
static bool is(token_type ch, T other) {
    if (auto* p = std::get_if<T>(&ch)) {
        return *p == other;
    }
    return false;
}

static bool match(char c, token_type ch) {
    if (is_char(ch)) {
        return std::get<char>(ch) == c;
    } else if (is_char_set(ch)) {
        const auto& set = std::get<char_set>(ch);
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

inline std::ostream& operator<<(std::ostream& os, const token_type& ch) {
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

static void print(const std::vector<token_type>& v) {
    for (const auto& ch : v) {
        std::cout << ch << ' ';
    }
    std::cout << std::endl;
}

static char_set to_char_set(const token_type& token) {
    if (std::holds_alternative<char>(token)) {
        return char_set{std::get<char>(token)};
    } else if (std::holds_alternative<char_set>(token)) {
        return std::get<char_set>(token);
    } else {
        throw std::invalid_argument("Invalid token type for conversion to char_set");
    }
}
} // namespace regex::token

#endif // REGEX_TOKEN_HPP