#pragma once
#ifndef GRAMMAR_LR1_HPP
#define GRAMMAR_LR1_HPP

#include "SLR.hpp"
#include "production.hpp"
#include <vector>

namespace grammar {
class LR1 : public SLR<production::LR1_production> {
public:
    explicit LR1(const std::vector<production::production>& productions) : SLR<production::LR1_production>(productions) {}

    explicit LR1(const std::string& str) : SLR<production::LR1_production>(str) {}

private:
    void init_first_item_set() override {
        items_t initial_items{production::LR1_production(productions[0], production::symbol::end_mark)};
        add_closure(initial_items, 0);
    }

    std::unordered_set<production::symbol> expand_item_set(const std::unordered_set<production::symbol>& symbols, items_t& current_item_set, const std::unordered_set<production::symbol>& after_dot) override {
        std::unordered_set<production::symbol> to_add;
        for (const auto& sym : symbols) {
            if (!sym.is_non_terminal()) {
                continue;
            }
            std::vector<production::symbol> lookaheads;
            symbol_set lookahead_set;
            auto calc_lookahead = [&](const production::LR1_production& item) {
                if (item.is_end()) {
                    return;
                }
                if (item.symbol_after_dot() != sym) {
                    return;
                }
                auto first = item.rhs.begin() + item.dot_pos + 1;
                if (first == item.rhs.end()) {
                    lookaheads.emplace_back(item.lookahead);
                    return;
                }
                std::vector<production::symbol> rhs{first, item.rhs.end()};
                rhs.emplace_back(item.lookahead);
                auto res = calc_first(rhs);
                for (const auto& r : res) {
                    if (!r.is_epsilon() && !lookahead_set.count(r)) {
                        lookaheads.emplace_back(r);
                        lookahead_set.insert(r);
                    }
                }
            };
            for (const auto& item : current_item_set) {
                calc_lookahead(item);
            }
            for (auto id : symbol_map[sym]) {
                auto prod = productions[id];
                for (std::size_t i = 0; i < lookaheads.size(); ++i) {
                    const auto& lookahead = lookaheads[i];
                    production::LR1_production lr_prod(prod, lookahead);
                    current_item_set.insert(lr_prod);
                    if (lr_prod.is_end()) {
                        continue;
                    }
                    const auto& after_dot_sym = lr_prod.symbol_after_dot();
                    if (!after_dot.count(after_dot_sym)) {
                        to_add.insert(after_dot_sym);
                    }
                    calc_lookahead(lr_prod);
                }
            }
        }
        return to_add;
    };

    void build_acc_and_reduce(const items_t& current_items, const std::size_t idx) override {
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
                    auto s = item.lookahead;
                    assert(!action_table[idx].count(s));
                    action_table[idx][s] = action::reduce(pid);
                }
            }
        }
    }
};
} // namespace grammar

#endif