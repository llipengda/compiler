#include "semantic/sema_production.hpp"
#include "utils.hpp"

#include <cassert>
#include <ranges>
#include <utility>

namespace semantic {

sema_symbol::sema_symbol() = default;

sema_symbol::sema_symbol(const std::string& str) : grammar::production::symbol(str) {}

#if __cplusplus >= 201703L
sema_symbol::sema_symbol(const std::string_view str) : grammar::production::symbol(std::string(str)) {}
#endif

sema_symbol::sema_symbol(const char* str) : grammar::production::symbol(std::string(str)) {}

std::ostream& operator<<(std::ostream& os, const sema_symbol& sym) {
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

void symbol_table::enter_scope_copy() {
    if (scopes.empty()) {
        scopes.emplace_back();
    } else {
        scopes.push_back(scopes.back());
        auto& current_scope = scopes.back();
        for (auto it = std::next(scopes.rbegin()); it != scopes.rend(); ++it) {
            for (auto& [key, value] : *it) {
                if (!current_scope.contains(key)) {
                    current_scope[key] = value;
                }
            }
        }
    }
}

void symbol_table::for_each_current(const std::function<void(const std::string&, const symbol_info&)>& func) const {
    if (!scopes.empty()) {
        const auto& current_scope = scopes.back();
        for (const auto& [key, value] : current_scope) {
            func(key, value);
        }
    }
}

void symbol_table::enter_scope() {
    scopes.emplace_back();
}

void symbol_table::exit_scope() {
    assert(!scopes.empty() && "No scope to exit");
    scopes.pop_back();
}

bool symbol_table::insert(const std::string& name, const symbol_info& info) {
    if (scopes.empty()) {
        enter_scope();
    }
    auto& current_scope = scopes.back();
    if (current_scope.contains(name)) {
        return false;
    }
    current_scope[name] = info;
    current_scope[name]["name"] = name;
    return true;
}

symbol_table::symbol_info* symbol_table::lookup(const std::string& name) {
    for (auto& scope : std::ranges::reverse_view(scopes)) {
        auto found = scope.find(name);
        if (found != scope.end()) {
            return &found->second;
        }
    }
    return nullptr;
}

void sema_env::error(const std::string& msg) {
    errors.emplace_back(msg);
}

sema_symbol& sema_env::symbol(const std::string& name) {
    return *symbols.back()[name];
}

void sema_env::enter_symbol_scope() {
    symbols.emplace_back();
}

void sema_env::add_symbol(const std::shared_ptr<sema_symbol>& sym) {
    auto& back = symbols.back();
    if (!back.contains(sym->name)) {
        back[sym->name] = sym;
        return;
    }
    int cnt = 1;
    for (const auto& key : back | std::views::keys) {
        if (utils::starts_with(key, sym->name + "<")) {
            cnt++;
        }
    }
    back[sym->name + "<" + std::to_string(cnt) + ">"] = sym;
}

void sema_env::exit_symbol_scope() {
    symbols.pop_back();
}

std::string sema_env::label() {
    return "L" + std::to_string(this->label_counter++);
}

std::string sema_env::temp() {
    return "__t" + std::to_string(this->temp_counter++);
}

void sema_env::emit(const std::string& code) {
    // Placeholder for actual code emission logic
    std::cout << "code: " << code << std::endl;
}

sema_production::rhs_value_t::rhs_value_t(symbol sym) : sym(std::move(sym)), is_symbol(true), is_action(false) {}

sema_production::rhs_value_t::rhs_value_t(action act) : act(std::move(act)), is_symbol(false), is_action(true) {}

sema_production::rhs_value_t::rhs_value_t(const char* str) : rhs_value_t(symbol(str)) {}

sema_production::rhs_value_t::rhs_value_t(const rhs_value_t& other) : is_symbol(other.is_symbol), is_action(other.is_action) {
    if (is_action) {
        new (&act) action(other.act);
    } else {
        new (&sym) symbol(other.sym);
    }
}

sema_production::rhs_value_t& sema_production::rhs_value_t::operator=(const rhs_value_t& other) {
    if (this != &other) {
        this->~rhs_value_t();
        new (this) rhs_value_t(other);
    }
    return *this;
}

sema_production::rhs_value_t::~rhs_value_t() {
    if (is_action) {
        act.~action();
    } else {
        sym.~symbol();
    }
}

sema_production::symbol& sema_production::rhs_value_t::get_symbol() {
    assert(is_symbol && "Attempting to get symbol from an action");
    return sym;
}

sema_production::action& sema_production::rhs_value_t::get_action() {
    assert(is_action && "Attempting to get action from a symbol");
    return act;
}

const sema_production::symbol& sema_production::rhs_value_t::get_symbol() const {
    assert(is_symbol && "Attempting to get symbol from an action");
    return sym;
}

const sema_production::action& sema_production::rhs_value_t::get_action() const {
    assert(is_action && "Attempting to get action from a symbol");
    return act;
}

std::ostream& operator<<(std::ostream& os, const sema_production::rhs_value_t& val) {
    if (val.is_symbol) {
        os << val.sym;
    } else if (val.is_action) {
        os << "[action]";
    }
    return os;
}

sema_production::operator grammar::production::production() const {
    grammar::production::production prod;
    prod.lhs = static_cast<grammar::production::symbol>(this->lhs);
    for (const auto& r : this->rhs) {
        if (r.is_symbol) {
            prod.rhs.emplace_back(r.get_symbol());
        }
    }
    return prod;
}

sema_production::sema_production() = default;

#ifdef SEMA_PROD_USE_INITIALIZER_LIST
sema_production::sema_production(std::initializer_list<rhs_value_t> values)
    : lhs(values.begin()->get_symbol()), rhs(values.begin() + 1, values.end()) {}
#endif

sema_production sema_production::replace(const grammar::production::symbol& sym) const {
    auto new_prod = *this;
    for (auto& r : new_prod.rhs) {
        if (r.is_symbol && r.get_symbol().name == sym.name) {
            r.get_symbol().lexval = sym.lexval;
        }
    }
    return new_prod;
}

} // namespace semantic