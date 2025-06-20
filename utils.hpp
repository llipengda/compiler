#pragma once
#include <ostream>
#include <unordered_map>
#ifndef UTILS_HPP
#define UTILS_HPP
#include <iostream>
#include <string>
#include <vector>

namespace utils {
inline std::string trim(const std::string& str) {
    const auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    const auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

inline std::string_view trim(std::string_view str) {
    const auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string_view::npos) return "";
    const auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

inline std::vector<std::string> split_trim(const std::string& s, const std::string& delimiter) {
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

inline std::vector<std::string_view> split_trim(std::string_view s, std::string_view delimiter) {
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

inline std::string join(const std::vector<std::string>& strings, const std::string& delimiter) {
    std::string result;
    for (const auto& s : strings) {
        if (!result.empty()) {
            result += delimiter;
        }
        result += s;
    }
    return result;
}

inline std::string join(const std::vector<std::string_view>& strings, const std::string_view& delimiter) {
    std::string result;
    for (const auto& s : strings) {
        if (!result.empty()) {
            result += delimiter;
        }
        result += s;
    }
    return result;
}

template <typename T>
inline void print(std::ostream& os, const std::vector<T>& v) {
    for (const auto& elem : v) {
        os << elem << " ";
    }
}

template <typename K, typename V>
inline void print(std::ostream& os, const std::unordered_map<K, V>& m) {
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
inline void print(T&& t) {
    print(std::cout, std::forward<T>(t));
}

template <typename T>
inline void println(std::ostream& os, T&& t) {
    print(os, std::forward<T>(t));
    os << '\n';
}

template <typename T>
inline void println(T&& t) {
    println(std::cout, std::forward<T>(t));
}
} // namespace utils

#endif