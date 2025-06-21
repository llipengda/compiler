#include <utility>

#include "grammar/SLR.hpp"

namespace grammar {

bool action::is_shift() const {
    return action_type == type::shift;
}

bool action::is_reduce() const {
    return action_type == type::reduce;
}

bool action::is_accept() const {
    return action_type == type::accept;
}

action action::shift(std::size_t val) {
    return {type::shift, val};
}

action action::reduce(std::size_t val) {
    return {type::reduce, val};
}

action action::accept() {
    return {type::accept, 0};
}

action action::error(std::size_t val) {
    return {type::error, val};
}

action::action() : action_type(type::error), val(-1) {}

action::action(type action_type, std::size_t val) : action_type(action_type), val(val) {}

std::ostream& operator<<(std::ostream& os, const action& act) {
    switch (act.action_type) {
    case action::type::shift:
        os << "s" + std::to_string(act.val);
        break;
    case action::type::reduce:
        os << "r" + std::to_string(act.val);
        break;
    case action::type::accept:
        os << "acc";
        break;
    case action::type::error:
        os << "e" + std::to_string(act.val);
        break;
    }
    return os;
}

LR_stack_t::LR_stack_t() : state(-1), type(type::state) {}

LR_stack_t::LR_stack_t(std::size_t s) : state(s), type(type::state) {}

LR_stack_t::LR_stack_t(production::symbol sym) : symbol(std::move(sym)), type(type::symbol) {}

LR_stack_t::LR_stack_t(const LR_stack_t& other) : type(other.type) {
    if (type == type::state) {
        state = other.state;
    } else {
        new (&symbol) production::symbol(other.symbol);
    }
}

LR_stack_t::LR_stack_t(LR_stack_t&& other) noexcept : type(other.type) {
    if (type == type::state) {
        state = other.state;
    } else {
        new (&symbol) production::symbol(std::move(other.symbol));
    }
}

LR_stack_t& LR_stack_t::operator=(const LR_stack_t& other) {
    if (this != &other) {
        this->~LR_stack_t();
        new (this) LR_stack_t(other);
    }
    return *this;
}

LR_stack_t& LR_stack_t::operator=(LR_stack_t&& other) noexcept {
    if (this != &other) {
        this->~LR_stack_t();
        new (this) LR_stack_t(std::move(other));
    }
    return *this;
}

LR_stack_t::~LR_stack_t() {
    if (is_symbol()) {
        symbol.~symbol();
    }
}

bool LR_stack_t::is_state() const {
    return type == type::state;
}

bool LR_stack_t::is_symbol() const {
    return type == type::symbol;
}

std::size_t LR_stack_t::get_state() const {
    assert(is_state());
    return state;
}

production::symbol LR_stack_t::get_symbol() const {
    assert(is_symbol());
    return symbol;
}

std::ostream& operator<<(std::ostream& os, const LR_stack_t& stack) {
    if (stack.is_state()) {
        os << stack.get_state();
    } else {
        os << stack.get_symbol();
    }
    return os;
}

void rightmost_step::add_step() {
    steps.emplace_back(symbols);
}

void rightmost_step::set_input(const std::vector<lexer::token>& input) {
    for (const auto& token : input) {
        symbols.emplace_back(token.value);
    }
    add_step();
}

void rightmost_step::add(const production::production& prod, const std::size_t ridx) {
    auto rit = symbols.rbegin() + ridx + prod.rhs.size() - 1;
    if (!(prod.rhs.size() == 1 && prod.rhs[0].is_epsilon())) {
        auto it = rit.base();
        it = symbols.erase(it, it + prod.rhs.size());
        symbols.insert(it, prod.lhs);
    } else {
        for (auto it = symbols.rbegin(); it != symbols.rend(); ++it) {
            if (it->is_non_terminal()) {
                symbols.insert(it.base(), prod.lhs);
                break;
            }
        }
    }
    add_step();
}

void rightmost_step::print() const {
    for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
        for (const auto& sym : *it) {
            std::cout << sym << " ";
        }
        if (std::next(it) != steps.rend()) {
            std::cout << "=> \n";
        }
    }
}

void rightmost_step::insert_symbol(const std::size_t ridx, const production::symbol& sym) {
    symbols.insert((symbols.rbegin() + ridx - 1).base(), sym);
    for (auto& step : steps) {
        step.insert((step.rbegin() + ridx - 1).base(), sym);
    }
}

} // namespace grammar