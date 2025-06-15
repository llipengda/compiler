#pragma once
#ifndef GRAMMAR_PRODUCTION_HPP
#define GRAMMAR_PRODUCTION_HPP

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
        if (auto remain = rhs_str.substr(start); !remain.empty()) {
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

inline std::ostream& operator<<(std::ostream& os, const production& prod) {
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

#endif // GRAMMAR_PRODUCTION_HPP