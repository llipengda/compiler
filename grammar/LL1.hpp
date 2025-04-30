#pragma once
#ifndef GRAMMAR_LL1_HPP
#define GRAMMAR_LL1_HPP

#include "exception.hpp"
#include "production.hpp"
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace grammar {
class grammar {
    virtual void parse(const std::vector<production::symbol>&) const = 0;
};

class LL1 : public grammar {
public:
    using symbol_set = std::unordered_set<production::symbol>;
    using table_t = std::unordered_map<production::symbol, std::unordered_map<production::symbol, production::production>>;

    LL1(const std::vector<production::production>& productions) : productions(productions) {
        for (std::size_t i = 0; i < productions.size(); ++i) {
            const auto& prod = productions[i];
            symbol_map[prod.lhs].push_back(i);
        }

        build();
    }

    LL1(const std::string& str) {
        productions = production::production::parse(str);
        for (std::size_t i = 0; i < productions.size(); ++i) {
            const auto& prod = productions[i];
            symbol_map[prod.lhs].push_back(i);
        }

        build();
    }

    LL1(const LL1&) = delete;
    LL1& operator=(const LL1&) = delete;
    LL1(LL1&&) = default;
    LL1& operator=(LL1&&) = default;

    void parse(const std::vector<production::symbol>& input) const override {
        std::vector<production::production> output;

        std::vector<production::symbol> in = input;
        in.push_back(production::symbol::end_mark);

        std::stack<production::symbol> stack;
        stack.push(production::symbol::end_mark);
        stack.push(productions[0].lhs);

        std::size_t pos = 0;

        while (pos < in.size() || !stack.empty()) {
            auto& cur_input = in[pos];
            auto& top = stack.top();

            if (top.is_terminal() || top.is_end_mark()) {
                if (top == cur_input) {
                    pos++;
                    stack.pop();
                } else {
                    stack.pop();
                    std::cerr << "Expect: " << top.name << " but got: " << cur_input.name << '\n';
                    // throw exception::grammar_error("Unexpected token: expected " + top.name + " instead of " + cur_input.name);
                }
            } else {
                const auto& table = parsing_table.at(top);
                if (!table.count(cur_input)) {
                    // throw exception::grammar_error("Unexpected token: " + cur_input.name);
                    if (first.at(top).count(production::symbol::epsilon)) {
                        stack.pop();
                    } else if (!follow.at(top).count(cur_input)) {
                        pos++;
                    } else {
                        throw exception::grammar_error("Unexpected token: " + cur_input.name);
                    }
                    std::cerr << "Unexpected token: " << cur_input.name << '\n';
                    continue;
                }
                stack.pop();
                const auto& prod = table.at(cur_input);
                output.push_back(prod);
                const auto& symbols = prod.rhs;

                if (symbols.size() == 1 && symbols[0].is_epsilon()) {
                    continue;
                }

                for (auto it = symbols.rbegin(); it != symbols.rend(); it++) {
                    stack.push(*it);
                }
            }
        }

        for (const auto& prod : output) {
            std::cout << prod << '\n';
        }
    }

    void print_first() const {
        std::cout << "FIRST sets:\n";
        for (const auto& [sym, first_set] : first) {
            if (sym.is_terminal()) {
                continue;
            }
            std::cout << "FIRST(" << sym << ") = {";
            for (auto it = first_set.begin(); it != first_set.end(); ++it) {
                std::cout << *it;
                if (std::next(it) != first_set.end()) {
                    std::cout << ',';
                }
            }
            std::cout << "}\n";
        }
        std::cout << '\n';
    }

    void print_follow() const {
        std::cout << "FOLLOW sets:\n";
        for (const auto& [sym, follow_set] : follow) {
            std::cout << "FOLLOW(" << sym << ") = {";
            for (auto it = follow_set.begin(); it != follow_set.end(); ++it) {
                std::cout << *it;
                if (std::next(it) != follow_set.end()) {
                    std::cout << ',';
                }
            }
            std::cout << "}\n";
        }
        std::cout << '\n';
    }

    void print_productions() const {
        std::cout << "Productions:\n";
        for (const auto& prod : productions) {
            std::cout << prod << '\n';
        }
        std::cout << '\n';
    }

    void print_parsing_table() const {
        std::cout << "Parsing Table:\n";
        for (const auto& [nt, table] : parsing_table) {
            for (const auto& [t, prod] : table) {
                std::cout << "M[" << nt << "," << t << "] = " << prod << '\n';
            }
        }
    }

    void print_parsing_table_pretty() const {
        auto longest_prod_len =
            std::max_element(
                productions.begin(),
                productions.end(),
                [](const auto& a, const auto& b) {
                    return a.to_string().size() < b.to_string().size();
                })
                ->to_string()
                .size();
        auto longest_non_terminal_sym_len =
            std::max_element(
                follow.begin(),
                follow.end(),
                [](const auto& a, const auto& b) {
                    return a.first.name.size() < b.first.name.size();
                })
                ->first.name.size();
        symbol_set terminals;
        symbol_set non_terminals;
        for (const auto& [sym, _] : first) {
            if ((sym.is_terminal() || sym.is_end_mark()) && !sym.is_epsilon()) {
                terminals.insert(sym);
            }
            if (sym.is_non_terminal()) {
                non_terminals.insert(sym);
            }
        }
        auto width = longest_non_terminal_sym_len + longest_prod_len * terminals.size() + (terminals.size() + 2);
        auto line = std::string(width, '-');
        std::cout << "Parsing Table:\n"
                  << line << '\n';
        std::cout << '|' << std::left << std::setw(longest_non_terminal_sym_len) << " " << '|';
        for (const auto& t : terminals) {
            std::cout << std::left << std::setw(longest_prod_len) << t << '|';
        }
        std::cout << '\n'
                  << line << '\n';
        for (const auto& nt : non_terminals) {
            std::cout << '|' << std::left << std::setw(longest_non_terminal_sym_len) << nt << '|';
            for (const auto& t : terminals) {
                if (parsing_table.at(nt).count(t)) {
                    const auto& prod = parsing_table.at(nt).at(t);
                    std::cout << std::left << std::setw(longest_prod_len) << prod << '|';
                } else {
                    std::cout << std::left << std::setw(longest_prod_len) << " " << '|';
                }
            }
            std::cout << '\n'
                      << line << '\n';
        }
    }

private:
    std::vector<production::production> productions;
    std::unordered_map<production::symbol, symbol_set> first;
    std::unordered_map<production::symbol, symbol_set> follow;

    std::unordered_map<production::symbol, std::vector<std::size_t>> symbol_map;
    table_t parsing_table;

    void build() {
        calc_first();
        calc_follow();
        build_parsing_table();
    }

    void calc_first() {
        for (const auto& prod : productions) {
            calc_first(prod.lhs);
        }
    }

    symbol_set& calc_first(production::symbol sym) {
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

    void build_parsing_table() {
        for (const auto& prod : productions) {
            auto first_set = calc_first(prod);
            for (const auto& sym : first_set) {
                if (sym.is_terminal() && !sym.is_epsilon()) {
                    if (parsing_table[prod.lhs].count(sym)) {
                        throw exception::ambiguous_grammar_exception({prod, parsing_table[prod.lhs][sym]});
                    }
                    parsing_table[prod.lhs][sym] = prod;
                }
            }
            if (first_set.count(production::symbol::epsilon)) {
                auto follow_set = follow[prod.lhs];
                for (const auto& sym : follow_set) {
                    if (sym.is_terminal() || sym.is_end_mark()) {
                        if (parsing_table[prod.lhs].count(sym)) {
                            throw exception::ambiguous_grammar_exception({prod, parsing_table[prod.lhs][sym]});
                        }
                        parsing_table[prod.lhs][sym] = prod;
                    }
                }
            }
        }
    }
};
} // namespace grammar

#endif // GRAMMAR_LL1_HPP