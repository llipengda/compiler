#include "grammar/LR1.hpp"

namespace grammar {

LR1::LR1(const std::vector<production::production>& productions) : SLR(productions) {}

LR1::LR1(const std::string& str) : SLR(str) {}

void LR1::init_first_item_set() {
    items_t initial_items{production::LR1_production(productions[0], production::symbol::end_mark)};
    add_closure(initial_items, 0);
}

std::unordered_set<production::symbol> LR1::expand_item_set(const std::unordered_set<production::symbol>& symbols, items_t& current_item_set, const std::unordered_set<production::symbol>& after_dot) {
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
            const auto first = item.rhs.begin() + item.dot_pos + 1;
            if (first == item.rhs.end()) {
                lookaheads.emplace_back(item.lookahead);
                return;
            }
            std::vector<production::symbol> rhs{first, item.rhs.end()};
            rhs.emplace_back(item.lookahead);
            for (const auto res = calc_first(rhs); const auto& r : res) {
                if (!r.is_epsilon() && !lookahead_set.contains(r)) {
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
            for (const auto& lookahead : lookaheads) {
                production::LR1_production lr_prod(prod, lookahead);
                current_item_set.insert(lr_prod);
                if (lr_prod.is_end()) {
                    continue;
                }
                if (const auto& after_dot_sym = lr_prod.symbol_after_dot(); !after_dot.contains(after_dot_sym)) {
                    to_add.insert(after_dot_sym);
                }
                calc_lookahead(lr_prod);
            }
        }
    }
    return to_add;
}

void LR1::build_acc_and_reduce(const items_t& current_items, const std::size_t idx) {
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
                auto s = item.lookahead;
                assert(!action_table[idx].contains(s));
                action_table[idx][s] = action::reduce(pid);
            }
        }
    }
}

} // namespace grammar