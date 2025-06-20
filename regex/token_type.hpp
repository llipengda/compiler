#pragma once

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
        for (const auto& [from, to] : init) {
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
#endif