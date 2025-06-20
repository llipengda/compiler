#pragma once
#include <memory>
#ifndef GRAMMAR_BASE_HPP
#define GRAMMAR_BASE_HPP

#include "../lexer/lexer.hpp"
#include "production.hpp"
#include "tree.hpp"
#include <stack>
#include <unordered_set>

namespace grammar {
class grammar_base {
public:
    using symbol_set = std::unordered_set<production::symbol>;
    virtual void parse(const std::vector<lexer::token>&) = 0;
    virtual void build() = 0;
    virtual ~grammar_base() = default;

    virtual void print_tree() const {
        tree_->print();
    }

    std::shared_ptr<tree> get_tree() const {
        return tree_;
    }

protected:
    std::vector<production::production> productions;
    std::unordered_map<production::symbol, symbol_set> first;
    std::unordered_map<production::symbol, symbol_set> follow;
    std::unordered_map<production::symbol, std::vector<std::size_t>> symbol_map;
    std::shared_ptr<tree> tree_ = std::make_shared<tree>();

    void calc_first() {
        for (const auto& prod : productions) {
            calc_first(prod.lhs);
        }
    }

    symbol_set& calc_first(const production::symbol& sym) {
        if (first.count(sym)) {
            return first[sym];
        }

        if (sym.is_terminal() || sym.is_epsilon()) {
            first[sym] = {sym};
            return first[sym];
        }

        auto& production_ids = symbol_map[sym];
        auto& result = first[sym];

        for (const auto& id : production_ids) {
            const auto& prod = productions[id];
            const auto& rhs = prod.rhs;

            for (const auto& s : rhs) {
                auto first_set = calc_first(s);
                result.insert(first_set.begin(), first_set.end());

                if (!first_set.count(production::symbol::epsilon)) {
                    break;
                }
            }
        }

        return result;
    }

    symbol_set calc_first(std::vector<production::symbol> symbols) {
        symbol_set result;
        for (const auto& sym : symbols) {
            auto first_set = calc_first(sym);
            result.insert(first_set.begin(), first_set.end());
            if (!first_set.count(production::symbol::epsilon)) {
                break;
            }
        }
        return result;
    }

    void calc_follow() {
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

        for (auto& [sym, follow_set] : follow) {
            follow_set.erase(production::symbol::epsilon);
        }
    }

    bool calc_follow(const std::size_t pos) {
        auto& prod = productions[pos];
        auto& rhs = prod.rhs;
        auto& lhs = prod.lhs;

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

        auto last = rhs.rbegin();
        if (last != rhs.rend()) {
            auto& sym = *last;
            if (sym.is_non_terminal()) {
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
            auto& first_set = calc_first(sym);
            if (first_set.count(production::symbol::epsilon)) {
                follow[prev_sym].insert(follow[lhs].begin(), follow[lhs].end());
            } else {
                break;
            }
        }

        return follow != follow_copy;
    }

    symbol_set calc_first(const production::production& prod) const {
        symbol_set result;
        for (const auto& sym : prod.rhs) {
            auto first_set = first.at(sym);
            result.insert(first_set.begin(), first_set.end());
            if (!first_set.count(production::symbol::epsilon)) {
                break;
            }
        }
        return result;
    }

    template <typename T>
    static void print_stack(const std::stack<T> stack) {
        std::vector<T> elements(stack.size());
        auto copy = stack;
        auto n = elements.size();
        for (std::size_t i = 0; i < n; ++i) {
            elements[n - i - 1] = copy.top();
            copy.pop();
        }
        for (const auto& elem : elements) {
            std::cout << elem << " ";
        }
        std::cout << std::endl;
    }
};
} // namespace grammar

#endif