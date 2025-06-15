#pragma once
#ifndef GRAMMAR_SLR_HPP
#define GRAMMAR_SLR_HPP

#include "exception.hpp"
#include "grammar_base.hpp"
#include "production.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
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

    [[nodiscard]] bool is_shift() const {
        return action_type == type::shift;
    }

    [[nodiscard]] bool is_reduce() const {
        return action_type == type::reduce;
    }

    [[nodiscard]] bool is_accept() const {
        return action_type == type::accept;
    }

    static action shift(std::size_t val) {
        return {type::shift, val};
    }

    static action reduce(std::size_t val) {
        return {type::reduce, val};
    }

    static action accept() {
        return {type::accept, 0};
    }

    static action error(std::size_t val) {
        return {type::error, val};
    }

    action() : action_type(type::error), val(-1) {}

private:
    action(type action_type, std::size_t val) : action_type(action_type), val(val) {}
};

inline std::ostream& operator<<(std::ostream& os, const action& act) {
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

struct LR_stack_t {
private:
    union {
        std::size_t state;
        production::symbol symbol;
    };

    enum class type {
        state,
        symbol
    } type;

public:
    LR_stack_t() : state(-1), type(type::state) {}
    LR_stack_t(std::size_t s) : state(s), type(type::state) {}
    LR_stack_t(const production::symbol& sym) : symbol(sym), type(type::symbol) {}

    LR_stack_t(const LR_stack_t& other) : type(other.type) {
        if (type == type::state) {
            state = other.state;
        } else {
            new (&symbol) production::symbol(other.symbol);
        }
    }

    LR_stack_t(LR_stack_t&& other) noexcept : type(other.type) {
        if (type == type::state) {
            state = other.state;
        } else {
            new (&symbol) production::symbol(std::move(other.symbol));
        }
    }

    LR_stack_t& operator=(const LR_stack_t& other) {
        if (this != &other) {
            this->~LR_stack_t();
            new (this) LR_stack_t(other);
        }
        return *this;
    }

    LR_stack_t& operator=(LR_stack_t&& other) noexcept {
        if (this != &other) {
            this->~LR_stack_t();
            new (this) LR_stack_t(std::move(other));
        }
        return *this;
    }

    ~LR_stack_t() {
        if (is_symbol()) {
            symbol.~symbol();
        }
    }

    [[nodiscard]] bool is_state() const { return type == type::state; }
    [[nodiscard]] bool is_symbol() const { return type == type::symbol; }

    [[nodiscard]] std::size_t get_state() const {
        assert(is_state());
        return state;
    }

    [[nodiscard]] production::symbol get_symbol() const {
        assert(is_symbol());
        return symbol;
    }
};

inline std::ostream& operator<<(std::ostream& os, const LR_stack_t& stack) {
    if (stack.is_state()) {
        os << stack.get_state();
    } else {
        os << stack.get_symbol();
    }
    return os;
}

struct rightmost_step {
private:
    std::vector<production::symbol> symbols;
    std::vector<std::vector<production::symbol>> steps;

    void add_step() {
        steps.emplace_back(symbols);
    }

public:
    rightmost_step() = default;

    void set_input(const std::vector<lexer::token>& input) {
        for (const auto& token : input) {
            symbols.emplace_back(token.value);
        }
        add_step();
    }

    void add(const production::production& prod, const std::size_t ridx) {
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

    void print() const {
        for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
            for (const auto& sym : *it) {
                std::cout << sym << " ";
            }
            if (std::next(it) != steps.rend()) {
                std::cout << "=> \n";
            }
        }
    }

    void insert_symbol(const std::size_t ridx, const production::symbol& sym) {
        symbols.insert((symbols.rbegin() + ridx - 1).base(), sym);
        for (auto& step : steps) {
            step.insert((step.rbegin() + ridx - 1).base(), sym);
        }
    }
};

template <typename Production = production::LR_production>
class SLR : public grammar_base {
public:
    using production_t = Production;
    // using production_t = production::LR_production; // for intellisense
    static_assert(std::is_base_of<production::LR_production, production_t>::value);

    explicit SLR(const std::vector<production::production>& productions) {
        this->productions = productions;

        const auto& first_prod = productions[0];
        this->productions.emplace(productions.begin(), first_prod.lhs.name + '\'' + " -> " + first_prod.lhs.name);

        for (std::size_t i = 0; i < productions.size(); ++i) {
            const auto& prod = productions[i];
            symbol_map[prod.lhs].push_back(i);
        }
    }

    explicit SLR(const std::string& str) {
        productions = production::production::parse(str);

        const auto& first_prod = productions[0];
        productions.emplace(productions.begin(), first_prod.lhs.name + '\'' + " -> " + first_prod.lhs.name);

        for (std::size_t i = 0; i < productions.size(); ++i) {
            const auto& prod = productions[i];
            symbol_map[prod.lhs].push_back(i);
        }

#ifdef SHOW_DEBUG
        std::cout << "Grammar:\n";
        for (const auto& prod : productions) {
            std::cout << prod << std::endl;
        }
#endif
    }

    void build() override {
        calc_first();
        calc_follow();
        build_items_set();
#ifdef SHOW_DEBUG
        print_items_set();
        print_tables();
#endif
    }

    void parse(const std::vector<lexer::token>& input) override {
        auto in = input;
        steps.set_input(in);
        in.emplace_back(production::symbol::end_mark_str);

        std::stack<LR_stack_t> stack;
        stack.push(std::size_t{0});

        std::vector<production::production> output;
        std::size_t pos = 0;

        while (pos < in.size() || !stack.empty()) {
            auto cur_input = production::symbol{std::string(in[pos])};
            auto& top = stack.top();

            assert(top.is_state());
            auto row = action_table.at(top.get_state());

            if (!row.count(cur_input)) {
                throw exception::grammar_error("Unexpected token: " + cur_input.name + " at line " + std::to_string(in[pos].line) + ", column " + std::to_string(in[pos].column));
            }
            auto act = row[cur_input];

#ifdef SHOW_DEBUG
            std::cout << "------------------------\n";
            std::cout << "stack: \n";
            print_stack(stack);
            std::cout << "input: ";
            for (std::size_t i = pos; i < in.size(); ++i) {
                std::cout << in[i].value << " ";
            }
            std::cout << "\n";
            std::cout << "action: " << act << '\n';
#endif
            if (act.is_accept()) {
                for (auto it = output.rbegin(); it != output.rend(); ++it) {
                    tree_.add_r(*it);
                }
                return;
            }

            if (act.is_shift()) {
                stack.push(cur_input);
                stack.push(act.val);
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
#ifdef SHOW_DEBUG
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

    void print_steps() const {
        steps.print();
    }

protected:
    using items_t = std::unordered_set<production_t>;

    std::vector<items_t> items_set;
    std::vector<symbol_set> after_dot_set;
    std::unordered_map<std::size_t, std::unordered_map<production::symbol, action>> action_table;
    std::unordered_map<std::size_t, std::unordered_map<production::symbol, std::size_t>> goto_table;
    rightmost_step steps;

    using error_handle_fn = std::function<void(std::stack<LR_stack_t>&, std::vector<lexer::token>&, std::size_t&)>;
    std::vector<error_handle_fn> error_handlers;

    virtual void init_first_item_set() {
        items_t initial_items{production_t(productions[0])};
        add_closure(initial_items, 0);
    }

    void build_items_set() {
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

        // for (auto& [row, col] : action_table) {
        //     if (!col.count(production::symbol{"}"})) {
        //         col[production::symbol{"}"}] = action::error(0);
        //     }
        // }

        // error_handlers.emplace_back([&](std::stack<LR_stack_t>& stack, std::vector<lexer::token>& in, std::size_t& pos) {
        //     std::cerr << "Missing semicolon at line " << in[pos].line << ", column " << in[pos].column << std::endl;
        //     in.insert(in.begin() + pos, lexer::token{";"});
        //     steps.insert_symbol(in.size() - pos - 1, production::symbol{";"});
        // });
    }

    /**
     * 扩展给定的符号集：对于每个非终结符，查找其产生式，将对应的LR(0)项插入当前项集，
     * 并收集新出现的点后的符号（未被after_dot包含的），用于后续扩展。
     *
     * @param symbols 需要扩展的符号集，通常是当前项集点后的符号。
     * @param current_item_set 当前LR(0)项集，会被插入新的项。
     * @param after_dot 已经考虑过的点后符号集，用于去重。
     * @return 新增的点后符号集（未被考虑过的），用于下一轮扩展。
     */
    virtual symbol_set expand_item_set(const symbol_set& symbols, items_t& current_item_set, const symbol_set& after_dot) {
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
    };

    virtual void build_acc_and_reduce(const items_t& current_items, const std::size_t idx) {
        for (const auto& item : current_items) {
            if (item.is_end()) {
                if (item.lhs == productions[0].lhs) {
                    assert(!action_table[idx].count(production::symbol::end_mark));
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

    /**
     * @brief 为给定的项集添加闭包到SLR分析器的项集集合中。
     *
     * 此函数针对指定索引（idx）在after_dot_set中的项集，计算其闭包。
     * 它会不断对项集应用闭包操作，直到没有新项可加入为止。
     * 如果生成的项集已存在于items_set中，则会删除after_dot_set中对应的条目，并返回已存在项集的索引。
     * 否则，将新项集加入items_set，并根据需要更新action_table（如accept或reduce动作）。
     *
     * @param current_items 当前需要扩展的项集的引用。
     * @param idx after_dot_set中对应项集的索引。
     * @return std::pair<bool, std::size_t>
     *         - 第一个元素为true表示找到新闭包或已存在闭包，false表示current_items为空。
     *         - 第二个元素为该项集在items_set中的索引，若未找到/添加则为-1。
     */
    virtual std::pair<bool, std::size_t> add_closure(items_t& current_items, std::size_t idx) {
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

    /**
     * @brief 在指定项集内，将点（dot）向指定符号 sym 后移，并生成新的项集。
     *
     * 该函数遍历索引为 idx 的项集中的所有项，查找点前为 sym 的项，将点向后移动一位，形成新的项集。
     * 对新项集中的每个项，如果点未到末尾，则收集点后的符号，形成 after_dot 集合。
     * 如果新项集非空，则为其添加闭包，并根据 sym 的类型（非终结符/终结符/结束符）更新 GOTO 或 ACTION 表。
     *
     * @param idx 当前项集在项集集合中的索引。
     * @param sym 要将点移过的文法符号。
     */
    virtual void move_dot(std::size_t idx, const production::symbol& sym) {
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

    void print_items_set() const {
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

    void print_tables() const {
        std::cout << "Parsing Table:\n";
        std::set<production::symbol> terminals;
        for (const auto& [state, row] : action_table) {
            for (const auto& [sym, act] : row) {
                terminals.insert(sym);
            }
        }

        std::set<production::symbol> non_terminals;
        for (const auto& [state, row] : goto_table) {
            for (const auto& [sym, target] : row) {
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
        for (const auto& [state, _] : action_table) {
            all_states.insert(state);
        }
        for (const auto& [state, _] : goto_table) {
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
};
} // namespace grammar

#endif