#pragma once
#ifndef SEMANTIC_SEMA_PRODUCTION_HPP
#define SEMANTIC_SEMA_PRODUCTION_HPP

#include "grammar/production.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

namespace semantic {
class sema_symbol final : public grammar::production::symbol {
public:
    std::unordered_map<std::string, std::string> syn{};
    std::unordered_map<std::string, std::string> inh{};

    sema_symbol();
    sema_symbol(const std::string& str);
#if __cplusplus >= 201703L
    sema_symbol(std::string_view str);
#endif
    sema_symbol(const char* str);

    friend std::ostream& operator<<(std::ostream& os, const sema_symbol& sym);
};

class symbol_table {
public:
    using symbol_info = std::unordered_map<std::string, std::string>;

    void enter_scope_copy();
    void for_each_current(const std::function<void(const std::string&, const symbol_info&)>& func) const;
    void enter_scope();
    void exit_scope();
    bool insert(const std::string& name, const symbol_info& info);
    symbol_info* lookup(const std::string& name);

private:
    std::vector<std::unordered_map<std::string, symbol_info>> scopes;
};

class sema_env {
public:
    std::vector<std::string> errors;
    std::vector<std::unordered_map<std::string, std::shared_ptr<sema_symbol>>> symbols;
    symbol_table table;

    void error(const std::string& msg);
    sema_symbol& symbol(const std::string& name);
    void enter_symbol_scope();
    void add_symbol(const std::shared_ptr<sema_symbol>& sym);
    void exit_symbol_scope();
};

class sema_production {
public:
    using symbol = semantic::sema_symbol;
    using action = std::function<void(sema_env& env)>;

    class rhs_value_t {
        union {
            symbol sym;
            action act;
        };

    public:
        bool is_symbol;
        bool is_action;

        rhs_value_t(symbol sym);
        rhs_value_t(action act);
        rhs_value_t(const char* str);
        rhs_value_t(const rhs_value_t& other);
        rhs_value_t& operator=(const rhs_value_t& other);
        ~rhs_value_t();

        symbol& get_symbol();
        action& get_action();
        const symbol& get_symbol() const;
        const action& get_action() const;

        friend std::ostream& operator<<(std::ostream& os, const rhs_value_t& val);
    };

    sema_symbol lhs;
    std::vector<rhs_value_t> rhs;

    explicit operator grammar::production::production() const;
    sema_production();

#ifdef SEMA_PROD_USE_INITIALIZER_LIST
    sema_production(std::initializer_list<rhs_value_t> values);
#else
#if __cplusplus >= 201703L
    template <typename... Args>
    sema_production(std::string_view lhs_str, Args&&... rhs_values);
#else
    template <typename... Args>
    sema_production(const std::string& lhs_str, Args&&... rhs_values);
#endif
#endif

    sema_production replace(const grammar::production::symbol& sym) const;
};

#define ACT(...) semantic::sema_production::rhs_value_t([](semantic::sema_env& env) { __VA_ARGS__ })
#define GET(x) auto& x = env.symbol(#x)
#define TO_STRING(x) #x
#define GETI(x, i) auto& x##_##i = env.symbol(TO_STRING(x<i>))

} // namespace semantic

#pragma region tpp

#ifndef SEMA_PROD_USE_INITIALIZER_LIST
#if __cplusplus >= 201703L
template <typename... Args>
semantic::sema_production::sema_production(const std::string_view lhs_str, Args&&... rhs_values)
    : lhs(lhs_str), rhs{rhs_value_t(std::forward<Args>(rhs_values))...} {}
#else
template <typename... Args>
semantic::sema_production::sema_production(const std::string& lhs_str, Args&&... rhs_values)
    : lhs(lhs_str), rhs{rhs_value_t(std::forward<Args>(rhs_values))...} {}
#endif
#endif

#pragma endregion

#endif