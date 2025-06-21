#include "regex/token_type.hpp"

namespace regex::token {

const std::unordered_map<symbol, std::string> symbol_map = {
    {symbol::end_mark, "#"}
};

const std::unordered_map<op, std::string> op_map = {
    {op::concat, "·"},
    {op::alt, "|"},
    {op::star, "*"},
    {op::plus, "+"},
    {op::left_par, "("},
    {op::right_par, ")"},
    {op::backslash, "\\"}
};

char_set::char_set() : is_negative(false) {}

char_set::char_set(const char from, const char to, const bool is_negative)
    : is_negative(is_negative) {
    for (char ch = from; ch <= to; ++ch) {
        chars.insert(ch);
    }
}

char_set::char_set(const std::initializer_list<char> init, bool is_negative)
    : is_negative(is_negative) {
    for (const auto& ch : init) {
        chars.insert(ch);
    }
}

char_set::char_set(std::initializer_list<std::pair<char, char>> init, const bool is_negative)
    : is_negative(is_negative) {
    for (const auto& [from, to] : init) {
        add(from, to);
    }
}

void char_set::add(const char ch) {
    chars.insert(ch);
}

void char_set::add(const char_set& other) {
    chars.insert(other.chars.begin(), other.chars.end());
}

void char_set::add(const char from, const char to) {
    for (char ch = from; ch <= to; ++ch) {
        chars.insert(ch);
    }
}

bool char_set::operator==(const char_set& other) const {
    return chars == other.chars && is_negative == other.is_negative;
}

const char_set words{{'a', 'z'}, {'A', 'Z'}, {'0', '9'}, {'_', '_'}};
const char_set digits('0', '9');
const char_set whitespaces{' ', '\t', '\n', '\r', '\f', '\v'};

#if __cplusplus < 201703L

storage::storage() {}
storage::~storage() {}

token_type::token_type() : type(kind::nothing) {
    new (&value._nothing) nothing{};
}

token_type::token_type(nothing) : type(kind::nothing) {
    new (&value._nothing) nothing{};
}

token_type::token_type(symbol s) : type(kind::symbol) {
    new (&value.sym) symbol(s);
}

token_type::token_type(op o) : type(kind::op) {
    new (&value.operation) op(o);
}

token_type::token_type(const char_set& cs) : type(kind::char_set) {
    new (&value.set) char_set(cs);
}

token_type::token_type(char c) : type(kind::character) {
    new (&value.character) char(c);
}

token_type::token_type(const token_type& other) : type(other.type) {
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

token_type::token_type(token_type&& other) noexcept : type(other.type) {
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

token_type::~token_type() {
    destroy();
}

token_type& token_type::operator=(const token_type& other) {
    if (this != &other) {
        destroy();
        new (this) token_type(other);
    }
    return *this;
}

token_type& token_type::operator=(token_type&& other) noexcept {
    if (this != &other) {
        destroy();
        new (this) token_type(std::move(other));
    }
    return *this;
}

bool token_type::operator==(const token_type& other) const {
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

void token_type::destroy() {
    switch (type) {
    case kind::char_set:
        value.set.~char_set();
        break;
    default:
        break;
    }
}

#endif

} // namespace regex::token
