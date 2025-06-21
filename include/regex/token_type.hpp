#pragma once
#ifndef REGEX_TOKEN_TYPE_HPP
#define REGEX_TOKEN_TYPE_HPP

#include <initializer_list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#if __cplusplus >= 201703L
#include <variant>
#endif

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

extern const std::unordered_map<symbol, std::string> symbol_map;
extern const std::unordered_map<op, std::string> op_map;

enum class nothing {};

struct char_set {
    std::unordered_set<char> chars;
    bool is_negative;

    char_set();
    explicit char_set(const char from, const char to, const bool is_negative = false);
    explicit char_set(const std::initializer_list<char> init, bool is_negative = false);
    explicit char_set(std::initializer_list<std::pair<char, char>> init, const bool is_negative = false);

    void add(const char ch);
    void add(const char_set& other);
    void add(const char from, const char to);

    bool operator==(const char_set& other) const;
};

extern const char_set words;
extern const char_set digits;
extern const char_set whitespaces;

#if __cplusplus >= 201703L
using token_type = std::variant<nothing, symbol, op, char_set, char>;
#else
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

        storage();
        ~storage();
    } value;

    token_type();
    token_type(nothing);
    token_type(symbol s);
    token_type(op o);
    token_type(const char_set& cs);
    token_type(char c);
    token_type(const token_type& other);
    token_type(token_type&& other) noexcept;
    ~token_type();

    token_type& operator=(const token_type& other);
    token_type& operator=(token_type&& other) noexcept;

    bool operator==(const token_type& other) const;

private:
    void destroy();
};
#endif

} // namespace regex::token

#if __cplusplus < 201703L
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
bool holds_alternative(const regex::token::token_type& t);

template <typename T>
T& get(regex::token::token_type& t);

template <typename T>
const T& get(const regex::token::token_type& t);

template <typename Visitor>
auto visit(Visitor&& vis, regex::token::token_type& t) -> decltype(auto);

template <typename Visitor>
auto visit(Visitor&& vis, const regex::token::token_type& t) -> decltype(auto);

template <typename T>
T* get_if(regex::token::token_type* t);

template <typename T>
const T* get_if(const regex::token::token_type* t);

} // namespace std
#endif

#pragma region tpp
#if __cplusplus < 201703L
namespace std {

template <typename T>
bool holds_alternative(const regex::token::token_type& t) {
    return t.type == token_type_traits<T>::value;
}

template <typename T>
T& get(regex::token::token_type& t) {
    if (!holds_alternative<T>(t)) {
        throw std::bad_cast();
    }

    if constexpr (std::is_same_v<T, regex::token::nothing>) {
        return (T&)(t.value._nothing);
    } else if constexpr (std::is_same_v<T, regex::token::symbol>) {
        return (T&)(t.value.sym);
    } else if constexpr (std::is_same_v<T, regex::token::op>) {
        return (T&)(t.value.operation);
    } else if constexpr (std::is_same_v<T, regex::token::char_set>) {
        return (T&)(t.value.set);
    } else if constexpr (std::is_same_v<T, char>) {
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

    if constexpr (std::is_same_v<T, nothing>) {
        return (T*)(&t->value._nothing);
    } else if constexpr (std::is_same_v<T, symbol>) {
        return (T*)(&t->value.sym);
    } else if constexpr (std::is_same_v<T, op>) {
        return (T*)(&t->value.operation);
    } else if constexpr (std::is_same_v<T, char_set>) {
        return (T*)(&t->value.set);
    } else if constexpr (std::is_same_v<T, char>) {
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
#pragma endregion

#endif