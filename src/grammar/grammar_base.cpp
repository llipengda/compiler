#include "grammar/grammar_base.hpp"

#include <ranges>

namespace grammar {

void grammar_base::print_tree() const {
    tree_->print();
}

std::shared_ptr<tree> grammar_base::get_tree() const {
    return tree_;
}

void grammar_base::calc_first() {
    for (const auto& prod : productions) {
        calc_first(prod.lhs);
    }
}

grammar_base::symbol_set& grammar_base::calc_first(const production::symbol& sym) {
    if (first.contains(sym)) {
        return first[sym];
    }

    if (sym.is_terminal() || sym.is_epsilon()) {
        first[sym] = {sym};
        return first[sym];
    }

    const auto& production_ids = symbol_map[sym];
    auto& result = first[sym];

    for (const auto& id : production_ids) {
        const auto& prod = productions[id];

        for (const auto& rhs = prod.rhs; const auto& s : rhs) {
            auto first_set = calc_first(s);
            result.insert(first_set.begin(), first_set.end());

            if (!first_set.contains(production::symbol::epsilon)) {
                break;
            }
        }
    }

    return result;
}

grammar_base::symbol_set grammar_base::calc_first(const std::vector<production::symbol>& symbols) {
    symbol_set result;
    for (const auto& sym : symbols) {
        auto first_set = calc_first(sym);
        result.insert(first_set.begin(), first_set.end());
        if (!first_set.contains(production::symbol::epsilon)) {
            break;
        }
    }
    return result;
}

void grammar_base::calc_follow() {
    follow[productions[0].lhs].insert(production::symbol::end_mark);
    while (true) {
        bool changed = false;
        for (std::size_t i = 0; i < productions.size(); ++i) {
            changed |= calc_follow(i);
        }
        if (!changed) {
            break;
        }
    }

    for (auto &follow_set: follow | std::views::values) {
        follow_set.erase(production::symbol::epsilon);
    }
}

bool grammar_base::calc_follow(const std::size_t pos) {
    auto& prod = productions[pos];
    auto& rhs = prod.rhs;
    const auto& lhs = prod.lhs;

    const auto follow_copy = follow;

    for (std::size_t i = 0; i < rhs.size(); ++i) {
        auto& sym = rhs[i];
        if (!sym.is_non_terminal()) {
            continue;
        }
        if (i + 1 < rhs.size()) {
            auto& next_sym = rhs[i + 1];
            auto& first_set = calc_first(next_sym);
            follow[sym].insert(first_set.begin(), first_set.end());
        }
    }

    if (const auto last = rhs.rbegin(); last != rhs.rend()) {
        if (const auto& sym = *last; sym.is_non_terminal()) {
            follow[sym].insert(follow[lhs].begin(), follow[lhs].end());
        }
    }

    for (auto it = rhs.rbegin(); it != rhs.rend(); ++it) {
        auto& sym = *it;
        if (std::next(it) == rhs.rend()) {
            continue;
        }
        auto& prev_sym = *std::next(it);

        if (!prev_sym.is_non_terminal()) {
            continue;
        }
        if (auto& first_set = calc_first(sym); first_set.contains(production::symbol::epsilon)) {
            follow[prev_sym].insert(follow[lhs].begin(), follow[lhs].end());
        } else {
            break;
        }
    }

    return follow != follow_copy;
}

grammar_base::symbol_set grammar_base::calc_first(const production::production& prod) const {
    symbol_set result;
    for (const auto& sym : prod.rhs) {
        auto first_set = first.at(sym);
        result.insert(first_set.begin(), first_set.end());
        if (!first_set.contains(production::symbol::epsilon)) {
            break;
        }
    }
    return result;
}

} // namespace grammar