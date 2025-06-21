#pragma once

#ifndef REGEX_TOKEN_HPP
#define REGEX_TOKEN_HPP

#include "token_type.hpp"

#include <iostream>
#include <ostream>
#include <string>
#include <vector>

namespace regex::token {

struct token_type_hash {
    std::size_t operator()(const token_type& t) const;
};

int get_precedence(op opr);
int get_precedence(const token_type& ch);

bool is_char(const token_type& ch);
bool is_symbol(const token_type& ch);
bool is_op(const token_type& ch);
bool is_char_set(const token_type& ch);

template <typename T>
bool is(token_type ch, T other);

bool match(char c, const token_type& ch);

bool is_nonop(const char ch);
bool is_nonop(const token_type& ch);

std::vector<token_type> split(const std::string& s);
std::vector<token_type> to_postfix(const std::vector<token_type>& v);

std::ostream& operator<<(std::ostream& os, const token_type& ch);

void print(const std::vector<token_type>& v);

char_set to_char_set(const token_type& token);

} // namespace regex::token

#pragma region tpp

namespace regex::token {

template <typename T>
inline bool is(token_type ch, T other) {
    if (auto* p = std::get_if<T>(&ch)) {
        return *p == other;
    }
    return false;
}

} // namespace regex::token

#pragma endregion

#endif // REGEX_TOKEN_HPP
