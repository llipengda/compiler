#pragma once
#ifndef GRAMMAR_PRODUCTION_HPP
#define GRAMMAR_PRODUCTION_HPP

#include <cctype>
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

    bool is_terminal() const {
        return type == type::terminal || type == type::epsilon;
    }

    bool is_non_terminal() const {
        return type == type::non_terminal;
    }

    bool is_epsilon() const {
        return type == type::epsilon;
    }

    bool is_end_mark() const {
        return type == type::end_mark;
    }

    bool operator==(const symbol& other) const {
        return type == other.type && name == other.name;
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

inline std::string symbol::epsilon_str = "ε";
inline std::string symbol::end_mark_str = "$";

inline std::function<bool(const std::string&)> symbol::terminal_rule = [](const std::string& str) {
    return !std::isupper(str[0]);
};

inline symbol symbol::epsilon = symbol(epsilon_str);
inline symbol symbol::end_mark = symbol(end_mark_str);

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
            rhs.push_back(symbol(s));
            start = end + 1;
        }
        auto remain = rhs_str.substr(start);
        if (!remain.empty()) {
            rhs.push_back(symbol(remain));
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const production& prod);

    static const std::vector<production> parse(const std::string& str) {
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
            std::string lhs_str = "";
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

    std::string to_string() const {
        std::string result = lhs.name + " -> ";
        for (const auto& sym : rhs) {
            result += sym.name + " ";
        }
        return result;
    }
};

inline std::ostream& operator<<(std::ostream& os, const production& prod) {
    os << prod.to_string();
    return os;
}
} // namespace grammar::production

namespace std {
template <>
struct hash<grammar::production::symbol> {
    std::size_t operator()(const grammar::production::symbol& sym) const {
        return std::hash<std::string>()(sym.name);
    }
};
} // namespace std

#endif // GRAMMAR_PRODUCTION_HPP