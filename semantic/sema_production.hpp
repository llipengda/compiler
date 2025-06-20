#pragma once
#ifndef SEMANTIC_SEMA_PRODUCTION_HPP
#define SEMANTIC_SEMA_PRODUCTION_HPP

#include "../grammar/production.hpp"
#include "../utils.hpp"
#include <cassert>
#include <functional>
#include <memory>
#include <ostream>
#include <unordered_map>

namespace semantic {
class sema_symbol final : public grammar::production::symbol {
public:
    std::unordered_map<std::string, std::string> syn{};
    std::unordered_map<std::string, std::string> inh{};

    sema_symbol() = default;

    sema_symbol(const std::string& str) : grammar::production::symbol(str) {}

    sema_symbol(const std::string_view str) : grammar::production::symbol(std::string(str)) {}

    sema_symbol(const char* str) : grammar::production::symbol(std::string(str)) {}

    friend std::ostream& operator<<(std::ostream& os, const sema_symbol& sym) {
        os << sym.name << "[" << "lexval=" << sym.lexval;
        if (!sym.syn.empty()) {
            os << ",syn=";
            utils::print(os, sym.syn);
        }
        if (!sym.inh.empty()) {
            os << ",inh=";
            utils::print(os, sym.inh);
        }
        os << "]";
        return os;
    }
};

class symbol_table {
public:
    using symbol_info = std::unordered_map<std::string, std::string>;

    void enter_scope_copy() {
        if (scopes.empty()) {
            scopes.emplace_back();
        } else {
            scopes.push_back(scopes.back());
            auto& current_scope = scopes.back();
            for (auto it = std::next(scopes.rbegin()); it != scopes.rend(); ++it) {
                for (auto& [key, value] : *it) {
                    if (!current_scope.count(key)) {
                        current_scope[key] = value;
                    }
                }
            }
        }
    }

    void for_each_current(const std::function<void(const std::string&, const symbol_info&)>& func) const {
        if (!scopes.empty()) {
            const auto& current_scope = scopes.back();
            for (const auto& [key, value] : current_scope) {
                func(key, value);
            }
        }
    }

    void enter_scope() {
        scopes.emplace_back();
    }

    void exit_scope() {
        assert(!scopes.empty() && "No scope to exit");
        scopes.pop_back();
    }

    bool insert(const std::string& name, const symbol_info& info) {
        if (scopes.empty()) {
            enter_scope();
        }
        auto& current_scope = scopes.back();
        if (current_scope.find(name) != current_scope.end()) {
            return false;
        }
        current_scope[name] = info;
        current_scope[name]["name"] = name;
        return true;
    }

    symbol_info* lookup(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto& scope = *it;
            auto found = scope.find(name);
            if (found != scope.end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unordered_map<std::string, symbol_info>> scopes;
};

class sema_env {
public:
    std::vector<std::string> errors;
    std::vector<std::unordered_map<std::string, std::shared_ptr<sema_symbol>>> symbols;

    symbol_table table;

    void error(const std::string& msg) {
        errors.emplace_back(msg);
    }

    sema_symbol& symbol(const std::string& name) {
        return *symbols.back()[name];
    }

    void enter_symbol_scope() {
        symbols.emplace_back();
    }

    void add_symbol(std::shared_ptr<sema_symbol> sym) {
        auto& back = symbols.back();
        if (!back.count(sym->name)) {
            back[sym->name] = sym;
            return;
        }
        int cnt = 1;
        for (const auto& [key, _] : back) {
            if (key.starts_with(sym->name + "<")) {
                cnt++;
            }
        }
        back[sym->name + "<" + std::to_string(cnt) + ">"] = sym;
    }

    void exit_symbol_scope() {
        symbols.pop_back();
    }
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

        rhs_value_t(const symbol& sym) : sym(sym), is_symbol(true), is_action(false) {}
        rhs_value_t(const action& act) : act(act), is_symbol(false), is_action(true) {}

        symbol& get_symbol() {
            assert(is_symbol && "Attempting to get symbol from an action");
            return sym;
        }

        action& get_action() {
            assert(is_action && "Attempting to get action from a symbol");
            return act;
        }

        const symbol& get_symbol() const {
            assert(is_symbol && "Attempting to get symbol from an action");
            return sym;
        }

        const action& get_action() const {
            assert(is_action && "Attempting to get action from a symbol");
            return act;
        }

        friend std::ostream& operator<<(std::ostream& os, const rhs_value_t& val) {
            if (val.is_symbol) {
                os << val.sym;
            } else if (val.is_action) {
                os << "[action]";
            }
            return os;
        }

        rhs_value_t(const rhs_value_t& other) : is_symbol(other.is_symbol), is_action(other.is_action) {
            if (is_action) {
                new (&act) action(other.act);
            } else {
                new (&sym) symbol(other.sym);
            }
        }

        rhs_value_t& operator=(const rhs_value_t& other) {
            if (this != &other) {
                this->~rhs_value_t();
                new (this) rhs_value_t(other);
            }
            return *this;
        }

        ~rhs_value_t() {
            if (is_action) {
                act.~action();
            } else {
                sym.~symbol();
            }
        }
    };

    sema_symbol lhs;
    std::vector<rhs_value_t> rhs;

    explicit operator grammar::production::production() const {
        grammar::production::production prod;
        prod.lhs = this->lhs;
        for (const auto& r : this->rhs) {
            if (r.is_symbol) {
                prod.rhs.emplace_back(r.get_symbol());
            }
        }
        return prod;
    }

    sema_production() = default;

    template <typename... Args>
    sema_production(const std::string_view lhs_str, Args&&... rhs_values)
        : lhs(lhs_str), rhs{rhs_value_t(std::forward<Args>(rhs_values))...} {}
        
    sema_production replace(const grammar::production::symbol& sym) {
        auto new_prod = *this;
        for (auto& r : new_prod.rhs) {
            if (r.is_symbol && r.get_symbol().name == sym.name) {
                r.get_symbol().lexval = sym.lexval;
            }
        }
        return new_prod;
    }
};

#define ACT(...) ([](semantic::sema_env& env) { __VA_ARGS__ })
#define GET(x) auto& x = env.symbol(#x)
#define TO_STRING(x) #x
#define GETI(x, i) auto& x##_##i = env.symbol(TO_STRING(x<i>))

} // namespace semantic

#endif