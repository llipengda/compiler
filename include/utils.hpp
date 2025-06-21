#pragma once
#include <stack>
#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#if __cplusplus >= 201703L
#include <string_view>
#endif

namespace utils {

std::string trim(const std::string& str);

#if __cplusplus >= 201703L
std::string_view trim(std::string_view str);
#endif

std::vector<std::string> split_trim(const std::string& s, const std::string& delimiter);

#if __cplusplus >= 201703L
std::vector<std::string_view> split_trim(std::string_view s, std::string_view delimiter);
#endif

std::string join(const std::vector<std::string>& strings, const std::string& delimiter);

#if __cplusplus >= 201703L
std::string join(const std::vector<std::string_view>& strings, const std::string_view& delimiter);
#endif

bool starts_with(const std::string& str, const std::string& prefix);

template <typename T>
void print(std::ostream& os, const std::vector<T>& v);

template <typename K, typename V>
void print(std::ostream& os, const std::unordered_map<K, V>& m);

template <typename T>
void print(std::ostream& os, const std::stack<T>& st);

template <typename T>
void print(T&& t);

template <typename T>
void println(std::ostream& os, T&& t);

template <typename T>
void println(T&& t);

} // namespace utils

#pragma region tpp
namespace utils {

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
void print(std::ostream& os, const std::stack<T>& st) {
    std::vector<T> elements(st.size());
    auto copy = st;
    auto n = elements.size();
    for (std::size_t i = 0; i < n; ++i) {
        elements[n - i - 1] = copy.top();
        copy.pop();
    }
    for (const auto& elem : elements) {
        os << elem << " ";
    }
}

template <typename T,
          std::enable_if_t<
              std::is_arithmetic_v<std::decay_t<T>>
                  || std::is_same_v<std::decay_t<T>, std::string>,
              int> = 0>
void print(std::ostream& os, T&& v) {
    os << v;
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

} // namespace utils
#pragma endregion

#endif // UTILS_HPP
