#pragma once
#ifndef GRAMMAR_PRODUCTION_HPP
#define GRAMMAR_PRODUCTION_HPP

#include "lexer/lexer.hpp"
#include <cstddef>
#include <functional>
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
    std::string lexval;

    std::size_t line = 0;
    std::size_t column = 0;

    static std::string epsilon_str;
    static std::string end_mark_str;
    static symbol epsilon;
    static symbol end_mark;
    static std::function<bool(const std::string&)> terminal_rule;

    symbol();
    explicit symbol(const std::string& str);
    explicit symbol(const lexer::token& token);

    void update(const lexer::token& token);
    void update(const symbol& other);
    void update_pos(const symbol& other);

    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] bool is_terminal() const;
    [[nodiscard]] bool is_non_terminal() const;
    [[nodiscard]] bool is_epsilon() const;
    [[nodiscard]] bool is_end_mark() const;

    bool operator==(const symbol& other) const;
    bool operator<(const symbol& other) const;
    bool operator!=(const symbol& other) const;

private:
    static std::string trim(const std::string& str);
};

std::ostream& operator<<(std::ostream& os, const symbol& sym);

class production {
public:
    symbol lhs;
    std::vector<symbol> rhs;

    production() = default;
    explicit production(const std::string& str);

    friend std::ostream& operator<<(std::ostream& os, const production& prod);

    static std::vector<production> parse(const std::string& str);

    [[nodiscard]] virtual std::string to_string() const;
    virtual bool operator==(const production& other) const;
    virtual ~production() = default;
};

std::ostream& operator<<(std::ostream& os, const production& prod);

class LR_production : public production {
public:
    std::size_t dot_pos{};

    LR_production() = default;
    explicit LR_production(const std::string& str);
    explicit LR_production(const production& prod);

    [[nodiscard]] bool is_start() const;
    [[nodiscard]] bool is_end() const;
    [[nodiscard]] bool is_rhs_empty() const;
    [[nodiscard]] LR_production next() const;
    [[nodiscard]] const symbol& symbol_after_dot() const;
    [[nodiscard]] std::string to_string() const override;

    bool operator==(const LR_production& other) const;
    bool operator==(const production& other) const override;
};

class LR1_production final : public LR_production {
public:
    symbol lookahead;

    LR1_production() = default;
    explicit LR1_production(const production& prod);
    explicit LR1_production(const production& prod, symbol&& lookahead);
    explicit LR1_production(const production& prod, const symbol& lookahead);

    void set_lookahead(const symbol& lookahead);
    [[nodiscard]] LR1_production next() const;
    [[nodiscard]] std::string to_string() const override;

    bool operator==(const LR1_production& other) const;
    bool operator==(const production& other) const override;
};

} // namespace grammar::production

namespace std {
template <>
struct hash<grammar::production::symbol> {
    std::size_t operator()(const grammar::production::symbol& sym) const noexcept {
        return std::hash<std::string>()(sym.name);
    }
};

template <>
struct hash<grammar::production::production> {
    std::size_t operator()(const grammar::production::production& prod) const noexcept {
        std::size_t h = std::hash<grammar::production::symbol>()(prod.lhs);
        for (const auto& sym : prod.rhs) {
            h ^= std::hash<grammar::production::symbol>()(sym) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};

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

namespace grammar {
void set_epsilon_str(const std::string& str);
void set_end_mark_str(const std::string& str);
void set_terminal_rule(const std::function<bool(const std::string&)>& rule);
void set_terminal_rule(std::function<bool(const std::string&)>&& rule);
} // namespace grammar

#endif // GRAMMAR_PRODUCTION_HPP