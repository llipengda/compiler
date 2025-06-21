#pragma once
#ifndef GRAMMAR_SLR_HPP
#define GRAMMAR_SLR_HPP

#include "exception.hpp"
#include "grammar_base.hpp"
#include "production.hpp"

#ifdef DEBUG
#include "utils.hpp"
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace grammar {

struct action {
    enum class type {
        shift,
        reduce,
        accept,
        error
    };

    type action_type;
    std::size_t val;

    [[nodiscard]] bool is_shift() const;
    [[nodiscard]] bool is_reduce() const;
    [[nodiscard]] bool is_accept() const;

    static action shift(std::size_t val);
    static action reduce(std::size_t val);
    static action accept();
    static action error(std::size_t val);

    action();

private:
    action(type action_type, std::size_t val);
};

std::ostream& operator<<(std::ostream& os, const action& act);

struct LR_stack_t {
private:
    union {
        std::size_t state{};
        production::symbol symbol;
    };

    enum class type {
        state,
        symbol
    } type;

public:
    LR_stack_t();
    LR_stack_t(std::size_t s);
    LR_stack_t(production::symbol sym);
    LR_stack_t(const LR_stack_t& other);
    LR_stack_t(LR_stack_t&& other) noexcept;
    LR_stack_t& operator=(const LR_stack_t& other);
    LR_stack_t& operator=(LR_stack_t&& other) noexcept;
    ~LR_stack_t();

    [[nodiscard]] bool is_state() const;
    [[nodiscard]] bool is_symbol() const;
    [[nodiscard]] std::size_t get_state() const;
    [[nodiscard]] production::symbol get_symbol() const;
};

std::ostream& operator<<(std::ostream& os, const LR_stack_t& stack);

struct rightmost_step {
private:
    std::vector<production::symbol> symbols;
    std::vector<std::vector<production::symbol>> steps;

    void add_step();

public:
    rightmost_step() = default;

    void set_input(const std::vector<lexer::token>& input);
    void add(const production::production& prod, std::size_t ridx);
    void print() const;
    void insert_symbol(std::size_t ridx, const production::symbol& sym);
};

template <typename Production = production::LR_production>
class SLR : public grammar_base {
public:
    using production_t = Production;
    static_assert(std::is_base_of_v<production::LR_production, production_t>);

    using action_table_t = std::unordered_map<std::size_t, std::unordered_map<production::symbol, action>>;
    using goto_table_t = std::unordered_map<std::size_t, std::unordered_map<production::symbol, std::size_t>>;
    using error_handle_fn = std::function<void(std::stack<LR_stack_t>&, std::vector<lexer::token>&, std::size_t&)>;

    explicit SLR(const std::vector<production::production>& productions_);
    explicit SLR(const std::string& str);

    void build() override;
    void parse(const std::vector<lexer::token>& input) override;
    void print_steps() const;
    void init_error_handlers(std::function<void(action_table_t&, goto_table_t&, std::vector<error_handle_fn>&)> fn);

protected:
    using items_t = std::unordered_set<production_t>;

    std::vector<items_t> items_set;
    std::vector<symbol_set> after_dot_set;
    action_table_t action_table;
    goto_table_t goto_table;
    rightmost_step steps;
    std::vector<error_handle_fn> error_handlers;
    std::function<void(action_table_t&, goto_table_t&, std::vector<error_handle_fn>&)> init_error_handlers_fn;

    virtual void init_first_item_set();
    void build_items_set();
    virtual symbol_set expand_item_set(const symbol_set& symbols, items_t& current_item_set, const symbol_set& after_dot);
    virtual void build_acc_and_reduce(const items_t& current_items, std::size_t idx);
    virtual std::pair<bool, std::size_t> add_closure(items_t& current_items, std::size_t idx);
    virtual void move_dot(std::size_t idx, const production::symbol& sym);
    void print_items_set() const;
    void print_tables() const;
};

} // namespace grammar

#pragma region tpp
namespace grammar {

template <typename Production>
SLR<Production>::SLR(const std::vector<production::production>& productions_) {
    productions = productions_;

    const auto& first_prod = productions_[0];
    productions.emplace(productions.begin(), first_prod.lhs.name + '\'' + " -> " + first_prod.lhs.name);

    for (std::size_t i = 0; i < productions.size(); ++i) {
        const auto& prod = productions[i];
        symbol_map[prod.lhs].push_back(i);
    }

#ifdef DEBUG
    std::cout << "Grammar:\n";
    for (const auto& prod : productions) {
        std::cout << prod << std::endl;
    }
#endif
}

template <typename Production>
SLR<Production>::SLR(const std::string& str) {
    productions = production::production::parse(str);

    const auto& first_prod = productions[0];
    productions.emplace(productions.begin(), first_prod.lhs.name + '\'' + " -> " + first_prod.lhs.name);

    for (std::size_t i = 0; i < productions.size(); ++i) {
        const auto& prod = productions[i];
        symbol_map[prod.lhs].push_back(i);
    }

#ifdef DEBUG
    std::cout << "Grammar:\n";
    for (const auto& prod : productions) {
        std::cout << prod << std::endl;
    }
#endif
}

template <typename Production>
void SLR<Production>::build() {
    calc_first();
    calc_follow();
    build_items_set();
#ifdef DEBUG
    print_items_set();
    print_tables();
#endif
}

template <typename Production>
void SLR<Production>::parse(const std::vector<lexer::token>& input) {
    auto in = input;
    steps.set_input(in);
    in.emplace_back(production::symbol::end_mark_str);

    std::stack<LR_stack_t> stack;
    stack.emplace(std::size_t{0});

    std::vector<production::production> output;
    std::size_t pos = 0;

    while (pos < in.size() || !stack.empty()) {
        auto cur_input = production::symbol{in[pos]};
        auto& top = stack.top();

        assert(top.is_state());
        auto row = action_table.at(top.get_state());

        if (!row.contains(cur_input)) {
            throw exception::grammar_error("Unexpected token: " + cur_input.name + " at line " + std::to_string(in[pos].line) + ", column " + std::to_string(in[pos].column));
        }
        auto act = row[cur_input];

#ifdef DEBUG
        std::cout << "------------------------\n";
        std::cout << "stack: \n";
        utils::println(stack);
        std::cout << "input: ";
        for (std::size_t i = pos; i < in.size(); ++i) {
            std::cout << in[i].value << " ";
        }
        std::cout << "\n";
        std::cout << "action: " << act << '\n';
#endif
        if (act.is_accept()) {
            for (const auto& prod : std::ranges::reverse_view(output)) {
                tree_->add_r(prod);
            }
            for (const auto& tk : in) {
                tree_->update_r(production::symbol{tk});
            }
            return;
        }

        if (act.is_shift()) {
            stack.emplace(cur_input);
            stack.emplace(act.val);
            pos++;
        } else if (act.is_reduce()) {
            const auto& prod = productions.at(act.val);
            auto r = prod.rhs.size();
            if (prod.rhs.size() == 1 && prod.rhs[0].is_epsilon()) {
                r = 0;
            }
            for (std::size_t i = 0; i < r; ++i) {
                stack.pop();
                stack.pop();
            }
            auto new_state = goto_table.at(stack.top().get_state()).at(prod.lhs);
            stack.push(prod.lhs);
            stack.push(new_state);
            output.emplace_back(prod);
            steps.add(prod, in.size() - pos);
#ifdef DEBUG
            std::cout << prod << '\n';
#endif
        } else {
            auto errid = act.val;
            if (errid < error_handlers.size()) {
                error_handlers[errid](stack, in, pos);
            } else {
                throw exception::grammar_error("Unexpected token: " + cur_input.name + " at line " + std::to_string(in[pos].line) + ", column " + std::to_string(in[pos].column));
            }
        }
    }
}

template <typename Production>
void SLR<Production>::print_steps() const {
    steps.print();
}

template <typename Production>
void SLR<Production>::init_error_handlers(std::function<void(action_table_t&, goto_table_t&, std::vector<error_handle_fn>&)> fn) {
    init_error_handlers_fn = std::move(fn);
}

template <typename Production>
void SLR<Production>::init_first_item_set() {
    items_t initial_items{production_t(productions[0])};
    add_closure(initial_items, 0);
}

template <typename Production>
void SLR<Production>::build_items_set() {
    items_set.reserve(productions.size() * 2);
    after_dot_set.reserve(productions.size() * 2);

    symbol_set after_dot;
    after_dot.insert(productions[0].rhs[0]);
    after_dot_set.emplace_back(std::move(after_dot));

    init_first_item_set();

    std::size_t init = 0;
    std::size_t end = items_set.size();
    while (init != end) {
        for (std::size_t i = init; i < end; i++) {
            after_dot = after_dot_set[i];
            for (const auto& sym : after_dot) {
                move_dot(i, sym);
            }
        }
        init = end;
        end = items_set.size();
    }

    if (init_error_handlers_fn) {
        init_error_handlers_fn(action_table, goto_table, error_handlers);
    }
}

template <typename Production>
typename SLR<Production>::symbol_set SLR<Production>::expand_item_set(const symbol_set& symbols, items_t& current_item_set, const symbol_set& after_dot) {
    symbol_set to_add;
    for (const auto& sym : symbols) {
        if (!sym.is_non_terminal()) {
            continue;
        }
        for (auto id : symbol_map[sym]) {
            auto lr_prod = production_t(productions[id]);
            current_item_set.insert(lr_prod);
            if (lr_prod.is_end()) {
                continue;
            }
            const auto& after_dot_sym = lr_prod.symbol_after_dot();
            if (!after_dot.count(after_dot_sym)) {
                to_add.insert(after_dot_sym);
            }
        }
    }
    return to_add;
}

template <typename Production>
void SLR<Production>::build_acc_and_reduce(const items_t& current_items, const std::size_t idx) {
    for (const auto& item : current_items) {
        if (item.is_end()) {
            if (item.lhs == productions[0].lhs) {
                assert(!action_table[idx].contains(production::symbol::end_mark));
                action_table[idx][production::symbol::end_mark] = action::accept();
            } else {
                std::size_t pid = -1;
                for (const auto id : symbol_map[item.lhs]) {
                    if (item == productions[id]) {
                        pid = id;
                        break;
                    }
                }
                assert(pid != -1);
                for (const auto& s : follow[item.lhs]) {
                    assert(!action_table[idx].count(s));
                    action_table[idx][s] = action::reduce(pid);
                }
            }
        }
    }
}

template <typename Production>
std::pair<bool, std::size_t> SLR<Production>::add_closure(items_t& current_items, std::size_t idx) {
    auto& after_dot = after_dot_set[idx];

    auto symbols = after_dot;
    bool is_empty;
    do {
        auto to_add = expand_item_set(symbols, current_items, after_dot);
        after_dot.insert(to_add.begin(), to_add.end());
        is_empty = to_add.empty();
        symbols = std::move(to_add);
    } while (!is_empty);

    if (current_items.empty()) {
        return {false, -1};
    }

    if (const auto res = std::find(items_set.begin(), items_set.end(), current_items); res != items_set.end()) {
        after_dot_set.erase(std::next(after_dot_set.begin(), idx));
        return {true, res - items_set.begin()};
    }

    build_acc_and_reduce(current_items, idx);

    items_set.emplace_back(std::move(current_items));
    return {true, items_set.size() - 1};
}

template <typename Production>
void SLR<Production>::move_dot(std::size_t idx, const production::symbol& sym) {
    const auto& items = items_set[idx];
    items_t new_items;
    symbol_set after_dot;
    for (const auto& item : items) {
        if (!item.is_end() && item.symbol_after_dot() == sym) {
            auto next_item = item.next();
            new_items.insert(next_item);
            if (!next_item.is_end()) {
                after_dot.insert(next_item.symbol_after_dot());
            }
        }
    }
    if (!new_items.empty()) {
        after_dot_set.emplace_back(std::move(after_dot));
        auto [success, to] = add_closure(new_items, after_dot_set.size() - 1);
        if (success) {
            if (sym.is_non_terminal()) {
                goto_table[idx][sym] = to;
            } else if (sym.is_terminal() || sym.is_end_mark()) {
                action_table[idx][sym] = action::shift(to);
            }
        }
    }
}

template <typename Production>
void SLR<Production>::print_items_set() const {
    int i = 0;
    for (const auto& items : items_set) {
        std::cout << "------------------------" << std::endl;
        std::cout << "I" << i++ << ":" << std::endl;
        for (const auto& item : items) {
            std::cout << item << std::endl;
        }
        std::cout << "After dot: ";
        for (const auto& sym : after_dot_set[i - 1]) {
            std::cout << sym << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "------------------------" << std::endl;
}

template <typename Production>
void SLR<Production>::print_tables() const {
    std::cout << "Parsing Table:\n";
    std::set<production::symbol> terminals;
    for (const auto& row : action_table | std::views::values) {
        for (const auto& sym : row | std::views::keys) {
            terminals.insert(sym);
        }
    }

    std::set<production::symbol> non_terminals;
    for (const auto& row : goto_table | std::views::values) {
        for (const auto& sym : row | std::views::keys) {
            non_terminals.insert(sym);
        }
    }

    std::cout << std::left << std::setw(8) << "state";
    for (const auto& sym : terminals) {
        std::cout << std::setw(8) << sym;
    }
    for (const auto& sym : non_terminals) {
        if (sym.name.size() > 5) {
            std::cout << std::setw(8) << sym.name.substr(0, 5) + "..";
        } else {
            std::cout << std::setw(8) << sym;
        }
    }
    std::cout << "\n";

    std::set<std::size_t> all_states;
    for (const auto& state : action_table | std::views::keys) {
        all_states.insert(state);
    }
    for (const auto& state : goto_table | std::views::keys) {
        all_states.insert(state);
    }

    for (std::size_t state : all_states) {
        std::cout << std::left << std::setw(8) << state;

        auto action_row_it = action_table.find(state);
        for (const auto& sym : terminals) {
            if (action_row_it != action_table.end()) {
                const auto& row = action_row_it->second;
                auto it = row.find(sym);
                if (it != row.end()) {
                    std::cout << std::setw(8) << it->second;
                } else {
                    std::cout << std::setw(8) << "";
                }
            } else {
                std::cout << std::setw(8) << "";
            }
        }

        auto goto_row_it = goto_table.find(state);
        for (const auto& sym : non_terminals) {
            if (goto_row_it != goto_table.end()) {
                const auto& row = goto_row_it->second;
                auto it = row.find(sym);
                if (it != row.end()) {
                    std::cout << std::setw(8) << it->second;
                } else {
                    std::cout << std::setw(8) << "";
                }
            } else {
                std::cout << std::setw(8) << "";
            }
        }

        std::cout << "\n";
    }
}

} // namespace grammar

#pragma endregion

#endif